"""
Visor del vector cardíaco v2 — acompaña a simulador_ecg_vector.ino
===================================================================
Qué muestra
  - Izquierda: DI, DII y DIII, lo mismo que leería un monitor de ECG en los bornes.
  - Centro:    el cuerpo visto de frente. Las tres pinzas (RA, LA, LL) se ARRASTRAN con el mouse.
               Al soltarlas, el programa calcula qué "ve" cada electrodo en su nueva posición
               y se lo manda a la ESP32, que cambia la señal de los bornes.
  - Derecha:   modelo 3D (se gira con el mouse) con la flecha del corazón y su recorrido.

Uso
  pip install pyserial matplotlib numpy
  python vector_ecg.py --demo                 sin placa: usa el mismo modelo que la ESP32
  python vector_ecg.py --puerto COM5          con la placa (cerrar antes el Monitor Serie)
                                              en Mac/Linux el puerto es algo como /dev/ttyUSB0

Teclas (con la ventana activa)
  0-5      escenario: 0 normal · 1 eje izquierdo · 2 eje derecho · 3 cables RA-LA invertidos
                      4 cables LA-LL invertidos · 5 dextrocardia
  <- / ->  gira el corazón de a 15 grados
  arriba / abajo   cambia el ritmo (40, 60, 120, 180 lpm)       c  calibración (1 mV, 1 Hz)
  r        devuelve las pinzas a su lugar

Modelo (docente, no anatómico)
  El corazón es un dipolo H(t) en el centro del pecho. Un electrodo en la posición p ve
      phi = L · H     con     L = (R0² / √3) · p / |p|³
  R0 es la distancia a las posiciones estándar (vértices del triángulo de Einthoven); ahí el
  modelo coincide exactamente con el ECG normal del simulador. Más cerca del corazón la señal
  crece; si no cabe en la salida, la ESP32 la reduce en proporción y avisa.

Importante: con el USB conectado al PC el simulador deja de estar aislado.
Para probar con un monitor de ECG real, usa el power bank y no este visor.
"""
import argparse, math, queue, re, threading, time
from collections import deque
import numpy as np

FS = 100                     # muestras por segundo que manda la ESP32
VENTANA = 3.0                # segundos visibles en las curvas

# =============================================================================
#  MODELO — las mismas cuentas que hace el firmware
# =============================================================================
#                      P      Q      R      S      T
AMP_DI  = np.array([0.10, -0.03, 0.80, -0.05, 0.35])
AMP_DII = np.array([0.15, -0.12, 1.00, -0.30, 0.20])
AMP_Z   = np.array([0.03,  0.08, -0.30, -0.25, 0.15])
V = np.column_stack([AMP_DI, (2 * AMP_DII - AMP_DI) / math.sqrt(3), AMP_Z])   # flecha de cada onda
EJE_BASE = math.degrees(math.atan2(V[2, 1], V[2, 0]))
U = np.array([[-0.5, -0.28868, 0.0], [0.5, -0.28868, 0.0], [0.0, 0.57735, 0.0]])  # RA, LA, LL
BORNES = ("RA", "LA", "LL")
ESCENARIOS = [   # nombre, eje pedido, espejo, qué electrodo del cuerpo queda en cada borne
    ("Normal",                            None, False, (0, 1, 2)),
    ("Desviacion del eje a la izquierda", -45,  False, (0, 1, 2)),
    ("Desviacion del eje a la derecha",   115,  False, (0, 1, 2)),
    ("Cables RA y LA invertidos",         None, False, (1, 0, 2)),
    ("Cables LA y LL invertidos",         None, False, (0, 2, 1)),
    ("Dextrocardia",                      None, True,  (0, 1, 2)),
]
BPM = (40, 60, 120, 180)
LIMITE_MV = 1.35


def corazon(bpm, eje=None, espejo=False):
    """La flecha H(t) de un latido, a 1 kHz. Devuelve (H de N x 3, índice de la onda R)."""
    giro = 0.0 if eje is None else math.radians(eje - EJE_BASE)
    co, si = math.cos(giro), math.sin(giro)
    v = np.column_stack([V[:, 0] * co - V[:, 1] * si, V[:, 0] * si + V[:, 1] * co, V[:, 2]])
    if espejo:
        v[:, 0] = -v[:, 0]
    RR = 60000 // bpm
    k = math.sqrt(RR / 1000.0); R = 0.40 * RR; t = np.arange(RR)
    centro = [R - 190 * k, R - 25, R, R + 25, R + 240 * k]
    ancho = [22 * k, 8, 9, 8, 40 * k]
    G = np.array([np.exp(-0.5 * ((t - c) / w) ** 2) for c, w in zip(centro, ancho)])   # 5 x N
    H = G.T @ v
    f = (t / (RR - 1))[:, None]
    H = H - (H[0] + (H[-1] - H[0]) * f)            # empalme suave entre latidos
    return H, int(R)


def proyectar(H, L):
    """DI y DII (mV) que entregan los bornes con los vectores de derivación L (3 x 3)."""
    di, dii = H @ (L[1] - L[0]), H @ (L[2] - L[0])
    pico = max(np.abs(di).max(), np.abs(dii).max())
    escala = LIMITE_MV / pico if pico > LIMITE_MV else 1.0
    return di * escala, dii * escala, escala


class Estado:
    """Réplica del firmware: mismo estado, mismos comandos, mismos mensajes."""

    def __init__(self):
        self.modo, self.plot = 1, 0
        self.aplicar(0)
        self.construir()

    def aplicar(self, n):
        self.esc = n
        _, self.eje, self.espejo, cable = ESCENARIOS[n]
        self.L = U[list(cable)].copy()
        self.libres = False

    def construir(self):
        if self.modo == 4:
            self.H, self.idxR = np.zeros((1000, 3)), 0
        else:
            self.H, self.idxR = corazon(BPM[self.modo], self.eje, self.espejo)
        return self.proyectar()

    def proyectar(self):
        if self.modo == 4:
            self.di = self.dii = np.where(np.arange(1000) < 500, 1.0, 0.0)
            self.escala = 1.0
        else:
            self.di, self.dii, self.escala = proyectar(self.H, self.L)
        return self.informe()

    def informe(self):
        nombre, eje0, _, cable = ESCENARIOS[self.esc]
        t = f"# Escenario {self.esc}: {nombre}"
        if self.eje is not None and self.eje != eje0:
            t += f" + eje {self.eje:.0f}"
        if self.libres:
            t += " + electrodos libres"
        t += " | CALIBRACION 1 Hz / 1 mV" if self.modo == 4 else f" | {BPM[self.modo]} lpm"
        out = [t]
        if self.libres:
            out.append("#   Electrodos: posicion libre (enviada desde el PC)")
        else:
            out.append("#   Electrodos: " + ", ".join(f"borne {b} <- {BORNES[c]}" for b, c in zip(BORNES, cable)))
        if self.modo != 4:
            a, b = self.di[self.idxR], self.dii[self.idxR]
            eje = math.degrees(math.atan2((2 * b - a) / math.sqrt(3), a))
            out.append(f"#   R en el monitor: DI {a:+.2f}  DII {b:+.2f}  DIII {b - a:+.2f} mV | eje aparente {eje:+.0f} grados")
        if self.escala < 1:
            out.append(f"# AVISO: senal reducida al {self.escala * 100:.0f} % para que quepa en la salida")
        return out

    def ejecutar(self, s):
        s = s.strip()
        if len(s) == 1 and s in "012345":
            self.aplicar(int(s))
            if self.modo == 4: self.modo = 1
            return self.construir()
        if s.startswith("eje"):
            self.eje = float(s[3:])
            if self.modo == 4: self.modo = 1
            return self.construir()
        if s.startswith("ritmo"):
            m = int(s[5:])
            if 0 <= m <= 4:
                self.modo = m
                return self.construir()
            return []
        if s.startswith("lv"):
            v = [float(x) for x in s[2:].split()]
            if len(v) != 9:
                return ["# lv necesita 9 numeros: x y z de RA, LA y LL"]
            self.L = np.array(v).reshape(3, 3)
            self.libres = True
            return self.proyectar()
        if s.startswith("plot"):
            self.plot = int(s[4:]) if s[4:].strip() else (0 if self.plot else 1)
            return []
        return ["# Comandos: 0-5 escenario | eje <grados> | ritmo <0-4> | plot <0-2> | ?"]


# =============================================================================
#  CUERPO — dónde está cada electrodo y qué vector de derivación le corresponde
# =============================================================================
R0 = 24.0                    # cm: distancia del corazón a las posiciones estándar
RMIN = 7.0                   # cm: no se permite acercar un electrodo más que esto al dipolo
ESTANDAR = U * math.sqrt(3) * R0          # RA (-20.8, -12, 0) · LA (20.8, -12, 0) · LL (0, 24, 0)
XM, A_T, B_T = -3.0, 16.0, 10.0           # torso: línea media, medio ancho y medio fondo (cm)
Y_SUP, Y_INF = -17.0, 22.0                # hombros y caderas (y crece hacia los pies)


def profundidad(x, y):
    """z de la piel del pecho en (x, y); 0 fuera del torso (brazos y piernas)."""
    u = (x - XM) / A_T
    if abs(u) >= 1 or y <= Y_SUP or y >= Y_INF:
        return 0.0
    s = min(1.0, min(y - Y_SUP, Y_INF - y) / 4.0)
    return B_T * math.sqrt(1 - u * u) * s * s * (3 - 2 * s)


def vector_derivacion(p):
    """L = (R0² / √3) · p / |p|³  (dipolo en un medio uniforme, calibrado en las posiciones estándar)."""
    p = np.asarray(p, float)
    r = np.linalg.norm(p)
    if r < 1e-6:
        return np.zeros(3)
    return (R0 ** 2 / math.sqrt(3)) * (p / r) / max(r, RMIN) ** 2


def comando_lv(pos):
    return "lv " + " ".join(f"{x:.4f}" for x in np.array([vector_derivacion(p) for p in pos]).ravel())


# =============================================================================
#  FUENTES DE DATOS — la placa por el puerto serie, o el modelo interno (demo)
# =============================================================================
RX_DATO = re.compile(r"DI:([-\d.]+),DII:([-\d.]+),DIII:[-\d.]+(?:,HX:([-\d.]+),HY:([-\d.]+),HZ:([-\d.]+))?")
RX_ESC = re.compile(r"# Escenario (\d): (.*)")
RX_R = re.compile(r"#\s+R en el monitor: (.*)")
RX_AVISO = re.compile(r"# AVISO: senal reducida al (\d+) %")


class Fuente:
    def __init__(self):
        n = int(FS * VENTANA)
        self.di, self.dii = deque([0.0] * n, maxlen=n), deque([0.0] * n, maxlen=n)
        self.h = deque([(0.0, 0.0, 0.0)] * n, maxlen=n)
        self.titulo, self.resumen = "Esperando datos...", ""
        self.esc, self.libres, self.eje, self.ritmo, self.escala = 0, False, EJE_BASE, 1, 1.0
        self.cambios = 0                 # sube cada vez que llega un encabezado de escenario
        self.muestras = 0

    def linea(self, txt):
        """Interpreta una línea que viene de la placa (o del modelo en demo)."""
        m = RX_DATO.match(txt)
        if m:
            a, b = float(m.group(1)), float(m.group(2))
            self.di.append(a); self.dii.append(b); self.muestras += 1
            if m.group(3) is not None:
                self.h.append((float(m.group(3)), float(m.group(4)), float(m.group(5))))
            else:                        # sin HX/HY/HZ: se reconstruye el plano frontal desde DI y DII
                self.h.append((a, (2 * b - a) / math.sqrt(3), 0.0))
            return
        m = RX_ESC.match(txt)
        if m:
            self.esc, resto = int(m.group(1)), m.group(2)
            self.titulo = resto
            self.libres = "electrodos libres" in resto
            e = re.search(r"\+ eje (-?\d+)", resto)
            eje0 = ESCENARIOS[self.esc][1]
            self.eje = float(e.group(1)) if e else (EJE_BASE if eje0 is None else float(eje0))
            r = re.search(r"\| (\d+) lpm", resto)
            self.ritmo = BPM.index(int(r.group(1))) if r and int(r.group(1)) in BPM else 4
            self.escala, self.resumen = 1.0, ""
            self.cambios += 1
            return
        m = RX_R.match(txt)
        if m:
            self.resumen = "R: " + m.group(1).replace("grados", "°").replace(" °", "°")
            return
        m = RX_AVISO.match(txt)
        if m:
            self.escala = int(m.group(1)) / 100.0


class Demo(Fuente):
    """Sin placa: el mismo modelo, generando 100 muestras por segundo."""

    def __init__(self):
        super().__init__()
        self.est = Estado()
        self.t0, self.n = time.monotonic(), 0
        for l in self.est.informe():
            self.linea(l)

    def enviar(self, cmd):
        for l in self.est.ejecutar(cmd):
            self.linea(l)

    def generar(self, cuantas):
        e = self.est
        for _ in range(cuantas):
            i = (self.n * 10) % len(e.di)
            self.linea("DI:%.3f,DII:%.3f,DIII:%.3f,HX:%.3f,HY:%.3f,HZ:%.3f"
                       % (e.di[i], e.dii[i], e.dii[i] - e.di[i], *e.H[i]))
            self.n += 1

    def actualizar(self):
        self.generar(min(int((time.monotonic() - self.t0) * FS) - self.n, FS))
        self.n = max(self.n, int((time.monotonic() - self.t0) * FS))


class Placa(Fuente):
    """La ESP32 por el puerto serie."""

    def __init__(self, puerto):
        super().__init__()
        import serial
        self.s = serial.Serial()
        self.s.port, self.s.baudrate, self.s.timeout = puerto, 115200, 0.1
        self.s.dtr = False; self.s.rts = False     # para que la base MB no reinicie ni bloquee la placa
        self.s.open()
        self.cola = queue.Queue()
        self.ultimo_dato = self.ultimo_plot = 0.0
        self.vivo = True
        threading.Thread(target=self._leer, daemon=True).start()
        self.enviar("plot 2")
        self.enviar("0")

    def _leer(self):
        while self.vivo:
            try:
                l = self.s.readline().decode(errors="ignore").strip()
            except Exception:
                break
            if l:
                self.cola.put(l)

    def enviar(self, cmd):
        self.s.write((cmd.strip() + "\n").encode())

    def actualizar(self):
        antes = self.muestras
        try:
            while True:
                self.linea(self.cola.get_nowait())
        except queue.Empty:
            pass
        ahora = time.monotonic()
        if self.muestras != antes:
            self.ultimo_dato = ahora
        elif ahora - self.ultimo_dato > 1.5 and ahora - self.ultimo_plot > 1.5:
            self.ultimo_plot = ahora               # la placa se reinició o aún no manda datos
            self.enviar("plot 2")

    def cerrar(self):
        self.vivo = False
        try:
            self.enviar("plot 0"); self.s.close()
        except Exception:
            pass


# =============================================================================
#  VENTANA
# =============================================================================
FONDO, TINTA, SUAVE, REJA = "#0f141a", "#e5e7eb", "#94a3b8", "#273240"
PIEL, BORDE = "#1a232e", "#3b4b5c"
C_I, C_II, C_III, C_V = "#f5d90a", "#22d3ee", "#e879f9", "#fb923c"
CM_POR_MV = 14.0             # largo de la flecha en el dibujo


class Visor:
    def __init__(self, fuente):
        import matplotlib.pyplot as plt
        from matplotlib.patches import Circle, FancyBboxPatch
        self.plt, self.f = plt, fuente
        self.pos = ESTANDAR.copy()               # posición (cm) de la pinza de cada borne
        self.agarrada, self.ultimo_lv, self.lv_pendiente, self.visto = None, 0.0, False, -1

        for k in ("keymap.back", "keymap.forward", "keymap.home", "keymap.yscale", "keymap.xscale"):
            plt.rcParams[k] = [x for x in plt.rcParams[k] if x not in ("left", "right", "r", "c", "l", "k")]
        plt.rcParams.update({"font.size": 9, "text.color": TINTA, "axes.labelcolor": SUAVE,
                             "xtick.color": SUAVE, "ytick.color": SUAVE})
        fig = self.fig = plt.figure(figsize=(15, 7), facecolor=FONDO)
        gs = fig.add_gridspec(3, 3, width_ratios=[1.25, 1, 1.05], left=0.05, right=0.99, top=0.86,
                              bottom=0.08, hspace=0.3, wspace=0.08)

        # --- curvas ---
        x = np.linspace(-VENTANA, 0, int(FS * VENTANA))
        self.curvas, self.ejes, self.lim = [], [], 1.5
        for fila, (nom, col) in enumerate((("DI", C_I), ("DII", C_II), ("DIII", C_III))):
            ax = fig.add_subplot(gs[fila, 0], facecolor=FONDO)
            ax.set_xlim(-VENTANA, 0); ax.set_ylim(-1.5, 1.5)
            ax.grid(color=REJA, lw=0.6); ax.axhline(0, color=REJA, lw=1)
            for s in ax.spines.values(): s.set_color(REJA)
            ax.set_ylabel(f"{nom}  (mV)", color=col)
            if fila < 2: ax.set_xticklabels([])
            else: ax.set_xlabel("tiempo (s)")
            self.curvas.append(ax.plot(x, np.zeros_like(x), color=col, lw=1.6)[0])
            self.ejes.append(ax)

        # --- cuerpo de frente ---
        ax = self.ax_c = fig.add_subplot(gs[:, 1], facecolor=FONDO)
        ax.set_aspect("equal"); ax.set_xlim(-36, 30); ax.set_ylim(54, -40); ax.axis("off")
        cuerpo = dict(fc=PIEL, ec=BORDE, lw=1.2)
        def caja(x0, y0, w, h, r): return FancyBboxPatch((x0, y0), w, h, boxstyle=f"round,pad=0,rounding_size={r}", **cuerpo)
        for p in (Circle((XM, -27.5), 7.5, **cuerpo), caja(XM - 3, -21, 6, 5, 1),
                  caja(XM - A_T - 8.5, Y_SUP + 1, 7.5, 36, 3.5), caja(XM + A_T + 1, Y_SUP + 1, 7.5, 36, 3.5),
                  caja(XM - A_T + 0.5, Y_INF + 1, A_T - 1, 28, 5), caja(XM + 0.5, Y_INF + 1, A_T - 1, 28, 5),
                  caja(XM - A_T, Y_SUP, 2 * A_T, Y_INF - Y_SUP, 5)):
            ax.add_patch(p)
        ax.text(-36, 52, "derecha\ndel paciente", color=SUAVE, fontsize=8, va="bottom")
        ax.text(30, 52, "izquierda\ndel paciente", color=SUAVE, fontsize=8, va="bottom", ha="right")
        ax.text(XM, -38.5, "Arrastra las pinzas", color=SUAVE, fontsize=9, ha="center")
        self.lados = [ax.plot([], [], color=c, lw=1.4, alpha=0.8)[0] for c in (C_I, C_II, C_III)]
        self.rastro = ax.plot([], [], color=C_V, lw=1.2, alpha=0.5)[0]
        ax.plot(0, 0, "o", color="#ef4444", ms=7, zorder=5)
        self.flecha = ax.annotate("", xy=(0, 0), xytext=(0, 0), zorder=6,
                                  arrowprops=dict(arrowstyle="-|>", color=C_V, lw=2.4, mutation_scale=16))
        self.pinzas = [ax.plot([], [], "o", ms=13, mfc="#e2e8f0", mec=FONDO, mew=1.5, zorder=7)[0] for _ in BORNES]
        self.rotulos = [ax.text(0, 0, b, ha="center", va="center", fontsize=7, fontweight="bold",
                                color=FONDO, zorder=8) for b in BORNES]

        # --- modelo 3D ---
        ax = self.ax_3 = fig.add_subplot(gs[:, 2], projection="3d", facecolor=FONDO)
        ax.set_axis_off(); ax.set_xlim(-26, 26); ax.set_ylim(-26, 26); ax.set_zlim(-30, 36)
        ax.set_box_aspect((52, 52, 66), zoom=1.75); ax.view_init(elev=12, azim=-68)
        t = np.linspace(0, 2 * np.pi, 49)
        for y in np.arange(Y_SUP, Y_INF + 0.1, 3.9):          # anillos del torso
            s = 1 - 0.12 * max(0, (y - 5) / (Y_INF - 5)) ** 2    # cintura un poco más angosta
            ax.plot(XM + A_T * s * np.cos(t), -B_T * s * np.sin(t), np.full_like(t, -y), color=BORDE, lw=0.7)
        for a in np.linspace(0, 2 * np.pi, 13)[:-1]:
            ax.plot([XM + A_T * math.cos(a)] * 2, [-B_T * math.sin(a)] * 2, [-Y_SUP, -Y_INF], color=BORDE, lw=0.5)
        for lat in np.linspace(-70, 70, 8):                    # cabeza
            r = 7.5 * math.cos(math.radians(lat))
            ax.plot(XM + r * np.cos(t), r * np.sin(t), np.full_like(t, 27.5 + 7.5 * math.sin(math.radians(lat))), color=BORDE, lw=0.6)
        ax.plot([0], [0], [0], "o", color="#ef4444", ms=6)
        self.lados3 = [ax.plot([], [], [], color=c, lw=1.2, alpha=0.8)[0] for c in (C_I, C_II, C_III)]
        self.rastro3 = ax.plot([], [], [], color=C_V, lw=1.2, alpha=0.6)[0]
        self.flecha3 = ax.plot([], [], [], color=C_V, lw=3, marker="o", ms=5, markevery=[1])[0]
        self.pinzas3 = [ax.plot([], [], [], "o", ms=8, mfc="#e2e8f0", mec=FONDO)[0] for _ in BORNES]
        self.rotulos3 = [ax.text(0, 0, 0, b, fontsize=8, fontweight="bold", color=TINTA) for b in BORNES]
        ax.text2D(0.5, -0.04, "modelo 3D · gíralo con el mouse", transform=ax.transAxes, ha="center", color=SUAVE)

        # --- textos ---
        self.titulo = fig.text(0.05, 0.945, "", fontsize=13, fontweight="bold")
        self.resumen = fig.text(0.05, 0.905, "", fontsize=10, color=SUAVE)
        self.aviso = fig.text(0.99, 0.905, "", fontsize=10, color=C_V, ha="right")
        fig.text(0.99, 0.95, "0–5 escenario · ← → girar · ↑ ↓ ritmo · c calibración · r reponer pinzas",
                 ha="right", color=SUAVE, fontsize=9)

        fig.canvas.mpl_connect("key_press_event", self.tecla)
        fig.canvas.mpl_connect("button_press_event", self.aprieta)
        fig.canvas.mpl_connect("motion_notify_event", self.mueve)
        fig.canvas.mpl_connect("button_release_event", self.suelta)
        self.dibujar_pinzas()

    # ---------------- pinzas ----------------
    def dibujar_pinzas(self):
        P = self.pos
        movida = [np.linalg.norm(P[b] - ESTANDAR[b]) > 0.01 for b in range(3)]
        for b in range(3):
            self.pinzas[b].set_data([P[b, 0]], [P[b, 1]])
            self.pinzas[b].set_markerfacecolor(C_V if movida[b] else "#e2e8f0")
            self.rotulos[b].set_position((P[b, 0], P[b, 1]))
            x, y, z = P[b, 0], -P[b, 2], -P[b, 1]
            self.pinzas3[b].set_data_3d([x], [y], [z])
            self.pinzas3[b].set_markerfacecolor(C_V if movida[b] else "#e2e8f0")
            self.rotulos3[b].set_position_3d((x + 1.5, y - 1.5, z + 2))
        for ln, ln3, (i, j) in zip(self.lados, self.lados3, ((0, 1), (0, 2), (1, 2))):   # I, II, III
            ln.set_data([P[i, 0], P[j, 0]], [P[i, 1], P[j, 1]])
            ln3.set_data_3d([P[i, 0], P[j, 0]], [-P[i, 2], -P[j, 2]], [-P[i, 1], -P[j, 1]])

    def colocar(self, b, x, y):
        x, y = float(np.clip(x, -34, 28)), float(np.clip(y, -36, 50))
        self.pos[b] = (x, y, profundidad(x, y))

    def mandar_lv(self):
        self.lv_pendiente = True
        self.ultimo_lv = time.monotonic()
        self.f.enviar(comando_lv(self.pos))

    def aprieta(self, ev):
        if ev.inaxes is not self.ax_c or ev.xdata is None: return
        d = [math.hypot(ev.xdata - p[0], ev.ydata - p[1]) for p in self.pos]
        if min(d) < 3.5:
            self.agarrada = int(np.argmin(d))

    def mueve(self, ev):
        if self.agarrada is None or ev.inaxes is not self.ax_c or ev.xdata is None: return
        self.colocar(self.agarrada, ev.xdata, ev.ydata)
        self.dibujar_pinzas()
        if time.monotonic() - self.ultimo_lv > 0.15:          # no más de ~7 envíos por segundo
            self.mandar_lv()

    def suelta(self, ev):
        if self.agarrada is None: return
        b, self.agarrada = self.agarrada, None
        for e in ESTANDAR:                                    # se "pega" a una posición estándar cercana
            if np.linalg.norm(self.pos[b, :2] - e[:2]) < 2.0:
                self.pos[b] = e
        self.dibujar_pinzas()
        self.mandar_lv()

    # ---------------- teclado ----------------
    def tecla(self, ev):
        f = self.f
        if ev.key in tuple("012345"):
            self.lv_pendiente = False; f.enviar(ev.key)
        elif ev.key in ("left", "right"):
            eje = (f.eje + (-15 if ev.key == "left" else 15) + 180) % 360 - 180
            f.eje = eje; f.enviar(f"eje {eje:.0f}")
        elif ev.key in ("up", "down"):
            r = 1 if f.ritmo == 4 else int(np.clip(f.ritmo + (1 if ev.key == "up" else -1), 0, 3))
            f.enviar(f"ritmo {r}")
        elif ev.key == "c":
            f.enviar("ritmo 4")
        elif ev.key == "r":
            self.lv_pendiente = False; f.enviar(str(f.esc))

    # ---------------- cada cuadro ----------------
    def cuadro(self, _=None):
        f = self.f
        f.actualizar()
        if f.cambios != self.visto:                           # llegó un escenario nuevo desde la placa
            self.visto = f.cambios
            if f.libres:
                self.lv_pendiente = False
            elif self.agarrada is None and not (self.lv_pendiente and time.monotonic() - self.ultimo_lv < 1.0):
                self.pos = ESTANDAR[list(ESCENARIOS[f.esc][3])].copy()
                self.dibujar_pinzas()
        di, dii = np.array(f.di), np.array(f.dii)
        for ln, y in zip(self.curvas, (di, dii, dii - di)):
            ln.set_ydata(y)
        pico = max(np.abs(di).max(), np.abs(dii).max(), np.abs(dii - di).max())
        lim = 1.5 if pico <= 1.45 else 3.0                    # DIII puede llegar al doble que un borne
        if lim != self.lim:
            self.lim = lim
            for ax in self.ejes: ax.set_ylim(-lim, lim)
        H = np.array(f.h)[-FS:] * CM_POR_MV                   # recorrido del último segundo
        self.rastro.set_data(H[:, 0], H[:, 1])
        self.flecha.xy = (H[-1, 0], H[-1, 1])
        self.rastro3.set_data_3d(H[:, 0], -H[:, 2], -H[:, 1])
        self.flecha3.set_data_3d([0, H[-1, 0]], [0, -H[-1, 2]], [0, -H[-1, 1]])
        self.titulo.set_text(f.titulo)
        self.resumen.set_text(f.resumen)
        self.aviso.set_text("" if f.escala >= 1 else f"señal reducida al {f.escala * 100:.0f} % para que quepa en la salida")

    def animar(self):
        from matplotlib.animation import FuncAnimation
        self._anim = FuncAnimation(self.fig, self.cuadro, interval=50, blit=False, cache_frame_data=False)
        self.plt.show()


def main():
    ap = argparse.ArgumentParser(description="Visor del vector cardíaco (simulador de ECG)")
    ap.add_argument("--puerto", help="puerto serie de la ESP32 (COM5, /dev/ttyUSB0...)")
    ap.add_argument("--demo", action="store_true", help="sin placa, con el modelo interno")
    ap.add_argument("--escenario", type=int, default=0, help="(demo) escenario inicial 0-5")
    ap.add_argument("--mover", action="append", default=[], metavar="BORNE=x,y",
                    help="(demo) coloca una pinza, en cm desde el corazón. Ej.: --mover LA=-6,-2")
    ap.add_argument("--png", help="(demo) guarda una imagen y termina")
    arg = ap.parse_args()
    if not arg.demo and not arg.puerto:
        ap.error("indica --puerto COMx o usa --demo")

    import matplotlib
    if arg.png:
        matplotlib.use("Agg")
    fuente = Demo() if arg.demo else Placa(arg.puerto)
    v = Visor(fuente)
    if arg.demo:
        if arg.escenario:
            fuente.enviar(str(arg.escenario)); v.cuadro()
        for m in arg.mover:
            b, xy = m.split("="); x, y = (float(t) for t in xy.split(","))
            v.colocar(BORNES.index(b.upper()), x, y)
        if arg.mover:
            v.dibujar_pinzas(); v.mandar_lv()
    if arg.png:
        e = fuente.est
        pico = int(np.argmax(np.linalg.norm(e.H, axis=1))) // 10
        fuente.generar(int(FS * VENTANA))
        ciclo = len(e.di) // 10
        while fuente.n % ciclo != (pico + 1) % ciclo:        # deja la flecha en la onda R
            fuente.generar(1)
        fuente.actualizar = lambda: None
        v.cuadro()
        v.fig.savefig(arg.png, dpi=140, facecolor=FONDO)
        return
    try:
        v.animar()
    finally:
        if hasattr(fuente, "cerrar"):
            fuente.cerrar()


if __name__ == "__main__":
    main()
