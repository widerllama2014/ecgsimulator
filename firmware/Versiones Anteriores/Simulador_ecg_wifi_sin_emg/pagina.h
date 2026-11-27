// pagina.h — la página web que entrega la ESP32-CAM. Va guardada dentro del chip y funciona sin internet.
// Para cambiar los datos de la ficha (carrera, curso, integrantes, logo) edita el bloque FICHA, unas líneas más abajo.
// Tipografía: "Press Start 2P" (SIL Open Font License 1.1), incrustada en la página.
#pragma once
#include <Arduino.h>

const char PAGINA[] PROGMEM = R"PAGINA(<!doctype html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<script>
/* ====== FICHA: los datos del proyecto se cambian aquí ====== */
const FICHA = {
  universidad: 'Universidad del Desarrollo',
  carrera:     'Ingeniería Civil en Biomedicina',
  curso:       'Electrónica Biomédica · IBB224S-1',
  proyecto:    'Simulador de ECG de 3 derivaciones',
  integrantes: 'Benjamín Garrido · José Tomás Romero',
  logo: ''     // para dejar el logo fijo, pega aquí la imagen como "data:image/png;base64,...."
};
</script>
<title>Panel Simulador ECG</title>
<meta name="claude:viewport-fit-sides" content="cover">
<style>
/* Dos apariencias: "clínica" (techo rojo, mostrador blanco) y "rayo" (negro y amarillo). Se cambia con el botón TEMA. */
/* Tipografía "Press Start 2P" (SIL Open Font License 1.1), incrustada para que funcione sin internet. */
@font-face { font-family: Pix; src: url(data:font/woff2;base64,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) format("woff2"); font-display: swap }
:root {
  --fondo: #fff6e9; --trama: rgba(34,37,59,.04); --panel: #ffffff; --texto: #22253b; --suave: #666a85; --borde: #22253b; --sombra: #22253b;
  --acento: #d93a3a; --acento2: #a32626; --sobre: #ffffff; --franja: linear-gradient(#fff, #fff); --ind: #a32626; --ind-on: #ffcf33;
  --tit-f: #22253b; --tit-t: #ffffff; --tit-s: #ffcf33; --sec: #8fdcc8; --sec-t: #22253b; --marco: #8fdcc8;
  --bandeja: #ffe3e6; --figura: #ffffff; --pz-f: #ffffff; --pz-t: #22253b; --pzm-f: #ffcf33; --pzm-t: #22253b;
  --pant: #0f201c; --reja: #21403a; --c1: #ffcf33; --c2: #6fe3ff; --c3: #ff8fd0; --vec: #ff7a3c;
  --l1: #c99400; --l2: #1596b8; --l3: #d2468f; --foco: #1596b8;
  color-scheme: light;
}
:root[data-tema="rayo"] {
  --fondo: #0d0e12; --trama: rgba(255,207,51,.035); --panel: #181a21; --texto: #f4f1e6; --suave: #a3a6b5; --borde: #ffcf33; --sombra: #7a5f00;
  --acento: #ffcf33; --acento2: #c79a00; --sobre: #0d0e12; --franja: repeating-linear-gradient(-45deg, #ffcf33 0 10px, #0d0e12 10px 20px); --ind: #ffcf33; --ind-on: #0d0e12;
  --tit-f: #ffcf33; --tit-t: #0d0e12; --tit-s: #5c4700; --sec: #2a3040; --sec-t: #f4f1e6; --marco: #ffcf33;
  --bandeja: #12141a; --figura: #262a35; --pz-f: #0d0e12; --pz-t: #ffcf33; --pzm-f: #ffcf33; --pzm-t: #0d0e12;
  --pant: #06080b; --reja: #232b36; --l1: #ffcf33; --l2: #6fe3ff; --l3: #ff8fd0; --foco: #6fe3ff;
  color-scheme: dark;
}
* { box-sizing: border-box }
body { margin: 0; color: var(--texto); font: 15px/1.45 system-ui, -apple-system, "Segoe UI", Roboto, sans-serif;
  background: var(--fondo) linear-gradient(90deg, var(--trama) 50%, transparent 50%) 0 0 / 48px 48px }
.techo { background: var(--acento); color: var(--sobre) }
.techo::after { content: ""; display: block; height: 10px; background: var(--franja); border-top: 5px solid var(--acento2); border-bottom: 3px solid var(--borde); box-sizing: content-box }
.techo .in, main, footer { max-width: 1180px; margin: 0 auto; padding-left: max(16px, env(safe-area-inset-left, 0px)); padding-right: max(16px, env(safe-area-inset-right, 0px)) }
.techo .in { display: flex; flex-wrap: wrap; align-items: center; gap: 12px 16px; padding-block: 16px 14px }
.cruz { width: 46px; height: 46px; background: #fff; border: 3px solid #22253b; display: grid; place-items: center; flex: none }
.cruz svg { width: 28px; height: 24px; transition: transform .08s }
.cruz.late svg { transform: scale(1.25) }
.marca { flex: 1 1 230px; min-width: 0 }
h1 { font: 400 15px/1.5 Pix, "Courier New", monospace; margin: 0; text-shadow: 2px 2px 0 var(--acento2) }
.techo p { margin: 3px 0 0; font-size: 13px }
.chip { background: var(--panel); color: var(--texto); border: 3px solid #22253b; padding: 8px 10px; font: 400 9px/1.4 Pix, monospace; display: flex; align-items: center; gap: 8px }
button.chip { cursor: pointer }
.luz { width: 10px; height: 10px; background: var(--suave); border: 2px solid var(--texto); flex: none }
.luz.ok { background: #35d07f; animation: parp 1.4s steps(2) infinite } .luz.demo { background: #ffcf33 } .luz.mal { background: #d93a3a }
@keyframes parp { 50% { opacity: .35 } }
main { padding-block: 24px 10px; display: grid; gap: 20px; grid-template-columns: minmax(0, 1fr) }
@media (min-width: 920px) { main { grid-template-columns: minmax(0, 1.2fr) minmax(0, 1fr) } .ancho { grid-column: 1 / -1 }
  .pant { flex: 1; min-height: 360px } #cvMon { position: absolute; inset: 0; height: 100%; aspect-ratio: auto } }
@media (pointer: coarse) { .tecl { display: none } }
.caja { background: var(--panel); border: 3px solid var(--borde); box-shadow: 5px 5px 0 var(--sombra); padding: 14px; min-width: 0; display: flex; flex-direction: column; gap: 12px }
h2 { font: 400 10px/1.4 Pix, monospace; margin: -14px -14px 0; padding: 9px 12px; min-height: 38px; background: var(--tit-f); color: var(--tit-t); display: flex; justify-content: space-between; align-items: center; gap: 8px }
h2 small { font: inherit; color: var(--tit-s); text-align: right }
/* ficha del proyecto */
.ficha { display: flex; flex-wrap: wrap; gap: 14px 22px; align-items: flex-start }
.logo { width: 96px; height: 96px; border: 3px dashed var(--suave); display: grid; place-items: center; text-align: center; font: 400 8px/1.7 Pix, monospace; color: var(--suave); flex: none; padding: 6px; background: transparent; cursor: pointer }
.logo.con { border: 0; padding: 4px; background: #fff } .logo img { max-width: 100%; max-height: 100%; display: block }
.ficha h3 { font: 400 13px/1.6 Pix, monospace; margin: 0 0 12px; color: var(--acento) }
:root[data-tema="rayo"] .ficha h3 { color: var(--texto) }
.fd { flex: 1 1 180px; min-width: 0 }
.datos { display: grid; grid-template-columns: repeat(auto-fit, minmax(min(200px, 100%), 1fr)); gap: 10px 22px; margin: 0 }
.datos dt { font: 400 8px/1.5 Pix, monospace; color: var(--ind); margin-bottom: 2px }
.datos dd { margin: 0; font-weight: 600 }
/* pantallas */
.pant { position: relative; background: var(--pant); border: 3px solid var(--borde) }
.pant::after { content: ""; position: absolute; inset: 0; pointer-events: none; background: repeating-linear-gradient(0deg, rgba(0,0,0,.16) 0 1px, transparent 1px 3px) }
canvas { display: block; width: 100%; touch-action: none }
#cvMon { aspect-ratio: 16 / 10 } #cvCuerpo { aspect-ratio: 66 / 80; max-height: 620px; cursor: grab; background: var(--bandeja); border: 3px solid var(--borde) }
.lect { display: grid; grid-template-columns: repeat(3, minmax(0, 1fr)); gap: 8px }
.lect div { border: 3px solid var(--borde); padding: 7px 8px; font: 400 9px/1.7 Pix, monospace; background: var(--pant); min-width: 0; overflow-wrap: anywhere }
.lect b { display: block; font-weight: 400 } .t1 { color: var(--c1) } .t2 { color: var(--c2) } .t3 { color: var(--c3) }
/* botones */
.b { font: 400 9px/1.5 Pix, monospace; color: var(--texto); background: var(--panel); border: 3px solid var(--borde); box-shadow: 3px 3px 0 var(--sombra); padding: 11px 9px; cursor: pointer; text-align: left; min-width: 0 }
.b:active { transform: translate(3px, 3px); box-shadow: none }
.b.on { background: var(--acento); color: var(--sobre) } .b.sec { background: var(--sec); color: var(--sec-t) }
.b:focus-visible, .chip:focus-visible, .logo:focus-visible, input:focus-visible, canvas:focus-visible { outline: 3px solid var(--foco); outline-offset: 2px }
.b i { font-style: normal; display: inline-block; min-width: 18px; color: var(--ind) } .b.on i { color: var(--ind-on) }
.mini { padding: 5px 7px; box-shadow: none; border-color: var(--tit-t); background: var(--tit-f); color: var(--tit-t) }
.mini.on { background: var(--tit-t); color: var(--tit-f) }
.rej { display: grid; gap: 10px; grid-template-columns: repeat(auto-fill, minmax(150px, 1fr)) }
.rej5 { display: grid; gap: 8px; grid-template-columns: repeat(5, minmax(0, 1fr)) } .rej5 .b { text-align: center; padding: 11px 2px }
.fila { display: flex; flex-wrap: wrap; gap: 10px; align-items: center }
label { font: 400 9px/1.6 Pix, monospace; display: grid; gap: 6px }
input[type=range] { width: 100%; accent-color: var(--acento); height: 24px }
.nota { font-size: 13px; color: var(--suave); margin: 0 }
/* cuadro de diálogo */
.dial { border: 3px solid var(--borde); box-shadow: inset 0 0 0 3px var(--panel), inset 0 0 0 6px var(--marco); padding: 16px 18px; font: 400 10px/2 Pix, monospace; min-height: 6.6em; background: var(--panel) }
.dial::after { content: "▼"; color: var(--acento); animation: parp 1s steps(2) infinite; margin-left: 6px }
.osc { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 8px }
.osc div { border: 3px solid var(--borde); padding: 8px 10px; background: var(--fondo) }
.osc span { font: 400 8px/1.6 Pix, monospace; color: var(--ind); display: block } .osc b { font: 600 17px/1.3 ui-monospace, "Cascadia Mono", Consolas, monospace }
footer { padding-block: 10px 34px; font-size: 12.5px; color: var(--suave) }
@media (prefers-reduced-motion: reduce) { .luz.ok, .dial::after { animation: none } .cruz svg { transition: none } }
</style>
</head>
<body>
<div class="techo"><div class="in">
  <div class="cruz" id="cruz" aria-hidden="true"><svg viewBox="0 0 7 6" shape-rendering="crispEdges"><path fill="#d93a3a" d="M1 0h2v1h1V0h2v1h1v2H6v1H5v1H4v1H3V5H2V4H1V3H0V1h1z"/><path fill="#fff" d="M1 1h1v1H1z"/></svg></div>
  <div class="marca">
    <h1>SIMULADOR DE ECG</h1>
    <p>3 derivaciones · ESP32-CAM · control por WiFi</p>
  </div>
  <button class="chip" id="tema" title="Cambiar la apariencia"></button>
  <div class="chip" role="status"><span class="luz" id="luz"></span><span id="conTxt">BUSCANDO PLACA</span></div>
</div></div>

<main>
  <section class="caja ancho">
    <h2>FICHA DEL PROYECTO</h2>
    <div class="ficha">
      <button class="logo" id="logo" title="Elegir la imagen del logo">LOGO<br>toca para elegir</button>
      <input type="file" id="logoArch" accept="image/*" hidden>
      <div class="fd">
        <h3 id="nomProy"></h3>
        <dl class="datos" id="datos"></dl>
      </div>
    </div>
  </section>

  <section class="caja">
    <h2>MONITOR <small id="monEsc">0,5 mV POR LINEA</small></h2>
    <div class="pant"><canvas id="cvMon" aria-label="Derivaciones DI, DII y DIII"></canvas></div>
    <div class="lect" id="lect"></div>
    <div class="dial" id="dial" aria-live="polite"></div>
  </section>

  <section class="caja">
    <h2>PACIENTE <span class="fila" style="gap:6px"><button class="b mini on" id="vF">FRENTE</button><button class="b mini" id="v3">3D</button></span></h2>
    <canvas id="cvCuerpo" aria-label="Cuerpo con las pinzas RA, LA y LL" tabindex="0"></canvas>
    <p class="nota" id="pista"></p>
  </section>

  <section class="caja">
    <h2>ESCENARIOS</h2>
    <div class="rej" id="escs"></div>
    <label>EJE DEL CORAZON: <span id="ejeTxt"></span>
      <input type="range" id="eje" min="-180" max="180" step="5" value="40"></label>
    <p class="nota tecl">Con teclado: 0 a 5 cambia el escenario, las flechas giran el corazón y cambian el ritmo, la barra espaciadora detiene la salida.</p>
  </section>

  <section class="caja">
    <h2>RITMO Y SALIDA</h2>
    <div class="rej5" id="ritmos"></div>
    <div class="fila">
      <button class="b sec" id="run"></button>
      <button class="b" id="reponer">REPONER PINZAS</button>
    </div>
    <div class="osc" id="osc"></div>
    <p class="nota">Valores para comparar con el osciloscopio: CH1 en el canal de LA y CH2 en el canal de LL, antes de la resistencia grande.</p>
  </section>
</main>
<footer>Fuente de prueba para equipos de adquisición y material de estudio. No se conecta a personas. Modelo docente: el cuerpo se trata como un medio uniforme.</footer>

<script>
(() => {
'use strict';
const $ = id => document.getElementById(id), S3 = Math.sqrt(3), mod = (a, n) => ((a % n) + n) % n;
const fmt = (x, d = 2) => x.toFixed(d).replace('.', ','), sg = (x, d = 2) => (x >= 0 ? '+' : '-') + fmt(Math.abs(x), d);
const guardar = (k, v) => { try { localStorage.setItem(k, v); } catch (e) {} }, leer = k => { try { return localStorage.getItem(k); } catch (e) { return null; } };
let PAL = {};
function paleta() { const c = getComputedStyle(document.documentElement); for (const n of ['texto', 'suave', 'borde', 'acento', 'pant', 'reja', 'c1', 'c2', 'c3', 'l1', 'l2', 'l3', 'vec', 'figura', 'pz-f', 'pz-t', 'pzm-f', 'pzm-t']) PAL[n] = c.getPropertyValue('--' + n).trim(); }

/* ====== MODELO: las mismas cuentas que hace la ESP32 ====== */
const AMP_DI = [0.10, -0.03, 0.80, -0.05, 0.35], AMP_DII = [0.15, -0.12, 1.00, -0.30, 0.20], AMP_Z = [0.03, 0.08, -0.30, -0.25, 0.15];
const V = AMP_DI.map((a, k) => [a, (2 * AMP_DII[k] - a) / S3, AMP_Z[k]]);
const EJE_BASE = Math.atan2(V[2][1], V[2][0]) * 180 / Math.PI, LIM = 1.35, BPM = [40, 60, 120, 180];
const U = [[-0.5, -0.28868, 0], [0.5, -0.28868, 0], [0, 0.57735, 0]], NOM = ['RA', 'LA', 'LL'];
const ESC = [
  { n: 'NORMAL', eje: null, esp: false, c: [0, 1, 2], t: 'Corazón normal, con el eje en +41°. DI y DII apuntan hacia arriba y DIII es pequeña.' },
  { n: 'EJE IZQUIERDO', eje: -45, esp: false, c: [0, 1, 2], t: 'Eje desviado a la izquierda (-45°). DI sigue hacia arriba; DII y DIII se dan vuelta.' },
  { n: 'EJE DERECHO', eje: 115, esp: false, c: [0, 1, 2], t: 'Eje desviado a la derecha (+115°). DI se da vuelta y DIII queda alta.' },
  { n: 'CABLES RA-LA', eje: null, esp: false, c: [1, 0, 2], t: 'Cables de los brazos puestos al revés. DI se invierte completa; DII y DIII intercambian papeles.' },
  { n: 'CABLES LA-LL', eje: null, esp: false, c: [0, 2, 1], t: 'Cables del brazo y la pierna izquierda al revés. DI y DII se intercambian; DIII se invierte.' },
  { n: 'DEXTROCARDIA', eje: null, esp: true, c: [0, 1, 2], t: 'Corazón en espejo, hacia la derecha. En las extremidades se ve igual que los cables de los brazos invertidos.' }];
const st = { esc: 0, eje: null, espejo: false, L: U.map(r => r.slice()), libres: false, modo: 1, corriendo: true, escala: 1 };
let lat = null;
function calcular() {
  let N, idxR, h, mA, mB, escala = 1;
  if (st.modo === 4) {
    N = 1000; idxR = 0; h = [0, 1, 2].map(() => new Float64Array(N)); mA = new Float64Array(N); mB = new Float64Array(N);
    for (let i = 0; i < 500; i++) mA[i] = mB[i] = 1;
  } else {
    const giro = st.eje === null ? 0 : (st.eje - EJE_BASE) * Math.PI / 180, co = Math.cos(giro), si = Math.sin(giro);
    const v = V.map(p => [(p[0] * co - p[1] * si) * (st.espejo ? -1 : 1), p[0] * si + p[1] * co, p[2]]);
    N = Math.floor(60000 / BPM[st.modo]); const k = Math.sqrt(N / 1000), R = 0.40 * N; idxR = Math.floor(R);
    const cen = [R - 190 * k, R - 25, R, R + 25, R + 240 * k], an = [22 * k, 8, 9, 8, 40 * k];
    h = [0, 1, 2].map(() => new Float64Array(N));
    for (let i = 0; i < N; i++) for (let w = 0; w < 5; w++) { const x = (i - cen[w]) / an[w], a = Math.exp(-0.5 * x * x); h[0][i] += a * v[w][0]; h[1][i] += a * v[w][1]; h[2][i] += a * v[w][2]; }
    for (const c of h) { const a = c[0], b = c[N - 1]; for (let i = 0; i < N; i++) c[i] -= a + (b - a) * i / (N - 1); }
    const L = st.L, a = [0, 1, 2].map(j => L[1][j] - L[0][j]), b = [0, 1, 2].map(j => L[2][j] - L[0][j]);
    mA = new Float64Array(N); mB = new Float64Array(N); let pico = 0;
    for (let i = 0; i < N; i++) { mA[i] = a[0] * h[0][i] + a[1] * h[1][i] + a[2] * h[2][i]; mB[i] = b[0] * h[0][i] + b[1] * h[1][i] + b[2] * h[2][i]; pico = Math.max(pico, Math.abs(mA[i]), Math.abs(mB[i])); }
    if (pico > LIM) escala = LIM / pico;
    for (let i = 0; i < N; i++) { mA[i] *= escala; mB[i] *= escala; }
  }
  st.escala = escala; lat = { N, idxR, h, mA, mB };
}
function local(s) {                               // intérprete de comandos para el modo demostración
  s = s.trim();
  if (/^[0-5]$/.test(s)) { const e = ESC[+s]; Object.assign(st, { esc: +s, eje: e.eje, espejo: e.esp, libres: false, L: e.c.map(c => U[c].slice()) }); if (st.modo === 4) st.modo = 1; }
  else if (s.startsWith('eje')) { st.eje = parseFloat(s.slice(3)); if (st.modo === 4) st.modo = 1; }
  else if (s.startsWith('ritmo')) { const m = parseInt(s.slice(5)); if (m >= 0 && m <= 4) st.modo = m; }
  else if (s.startsWith('run')) st.corriendo = s.slice(3).trim() === '1';
  else if (s.startsWith('lv')) { const v = s.slice(2).trim().split(/\s+/).map(Number); st.L = [v.slice(0, 3), v.slice(3, 6), v.slice(6, 9)]; st.libres = true; }
}

/* ====== CUERPO: dónde están las pinzas y qué ve cada una ====== */
const R0 = 24, RMIN = 7, XM = -3, A_T = 16, B_T = 10, Y_SUP = -17, Y_INF = 22, EST = U.map(u => u.map(c => c * S3 * R0)), CM = 14, KD = R0 * R0 / S3;
let pos = EST.map(p => p.slice()), agarrada = null, vista = 'f', az = -0.38, el = 0.2, giro3 = null;
function prof(x, y) { const u = (x - XM) / A_T; if (Math.abs(u) >= 1 || y <= Y_SUP || y >= Y_INF) return 0; const s = Math.min(1, Math.min(y - Y_SUP, Y_INF - y) / 4); return B_T * Math.sqrt(1 - u * u) * s * s * (3 - 2 * s); }
function vecDer(p) { const r = Math.hypot(...p); if (r < 1e-6) return [0, 0, 0]; const k = KD / (r * Math.max(r, RMIN) ** 2); return p.map(c => c * k); }
function posDe(L) {                               // el camino inverso: de lo que ve la pinza a dónde está
  const m = Math.hypot(...L); if (m < 1e-9) return [0, 0, 0];
  const r = m >= KD / (RMIN * RMIN) ? RMIN : Math.sqrt(KD / m), p = L.map(c => c / m * r);
  for (const e of EST) if (Math.hypot(p[0] - e[0], p[1] - e[1], p[2] - e[2]) < 0.3) return e.slice();
  return p;
}
const cmdLv = () => 'lv ' + pos.map(p => vecDer(p).map(x => x.toFixed(4)).join(' ')).join(' ');

/* ====== CONEXIÓN CON LA PLACA ====== */
let modoCon = 'buscando', fase = { idx: 0, t: performance.now() }, cola = Promise.resolve(), ultEnvio = 0, pend = 0;
async function pedir(url, sondeo) {
  const ctl = new AbortController(), tm = setTimeout(() => ctl.abort(), 2500), t0 = performance.now();
  try {
    const r = await fetch(url, { cache: 'no-store', signal: ctl.signal }); if (!r.ok) throw 0; const j = await r.json(), t = (t0 + performance.now()) / 2;
    modoCon = 'placa'; fase = { idx: j.idx, t };
    if (sondeo && (pend > 0 || agarrada !== null)) return;      // hay un cambio propio en camino: manda ese
    Object.assign(st, { esc: j.esc, eje: j.eje, espejo: j.espejo, libres: j.libres, modo: j.modo, corriendo: j.corriendo });
    if (agarrada === null) {
      st.L = [j.L.slice(0, 3), j.L.slice(3, 6), j.L.slice(6, 9)];
      if (st.libres && pos.some((p, b) => vecDer(p).some((x, k) => Math.abs(x - st.L[b][k]) > 2e-4))) pos = st.L.map(posDe);   // la movieron desde otro equipo
    }
    calcular(); reflejar();
  } finally { clearTimeout(tm); }
}
function mandar(cmd) {
  if (modoCon === 'demo') { local(cmd); calcular(); reflejar(); return; }
  pend++; ultEnvio = performance.now();
  cola = cola.then(() => pedir('/api/cmd?c=' + encodeURIComponent(cmd))).catch(() => { if (modoCon === 'placa') modoCon = 'caida'; pintarCon(); }).finally(() => pend--);
}
function pintarCon() {
  const [c, t] = { placa: ['ok', 'PLACA CONECTADA'], demo: ['demo', 'MODO DEMOSTRACION'], caida: ['mal', 'SIN CONEXION'], buscando: ['', 'BUSCANDO PLACA'] }[modoCon];
  $('luz').className = 'luz ' + c; $('conTxt').textContent = t;
}
const faseMs = () => modoCon === 'placa' || modoCon === 'caida' ? mod(fase.idx + performance.now() - fase.t, lat.N) : mod(performance.now(), lat.N);

/* ====== DIBUJO DEL PACIENTE ====== */
function lienzo(cv) { const r = cv.getBoundingClientRect(), d = devicePixelRatio || 1, w = Math.max(40, Math.round(r.width)), h = Math.max(40, Math.round(r.height)); if (cv.width !== Math.round(w * d) || cv.height !== Math.round(h * d)) { cv.width = Math.round(w * d); cv.height = Math.round(h * d); } const g = cv.getContext('2d'); g.setTransform(d, 0, 0, d, 0, 0); return { g, w, h }; }
function caja(g, x, y, w, h, r) { g.beginPath(); g.roundRect ? g.roundRect(x, y, w, h, r) : g.rect(x, y, w, h); g.fill(); g.stroke(); }
function dibujarCuerpo(ms) {
  const { g, w, h } = lienzo($('cvCuerpo')), s = Math.min(w / 66, h / 80);
  g.clearRect(0, 0, w, h);
  const P = vista === 'f' ? p => [w / 2 + s * (p[0] + 3), h / 2 + s * (p[1] - 3)] : p => { const X = p[0] - XM, Y = -(p[1] + 4), Z = p[2], x1 = X * Math.cos(az) + Z * Math.sin(az), z1 = -X * Math.sin(az) + Z * Math.cos(az); return [w / 2 + s * 1.15 * x1, h / 2 - s * 1.15 * (Y * Math.cos(el) + z1 * Math.sin(el))]; };
  g.lineWidth = 2.5; g.fillStyle = PAL.figura; g.strokeStyle = PAL.borde; g.lineJoin = 'round';
  if (vista === 'f') {
    const X = x => w / 2 + s * (x + 3), Y = y => h / 2 + s * (y - 3);
    g.beginPath(); g.arc(X(XM), Y(-27.5), 7.5 * s, 0, 7); g.fill(); g.stroke(); caja(g, X(XM - 3), Y(-21), 6 * s, 5 * s, s);
    caja(g, X(XM - A_T - 8.5), Y(Y_SUP + 1), 7.5 * s, 36 * s, 3.5 * s); caja(g, X(XM + A_T + 1), Y(Y_SUP + 1), 7.5 * s, 36 * s, 3.5 * s);
    caja(g, X(XM - A_T + 0.5), Y(Y_INF + 1), (A_T - 1) * s, 24 * s, 5 * s); caja(g, X(XM + 0.5), Y(Y_INF + 1), (A_T - 1) * s, 24 * s, 5 * s);
    caja(g, X(XM - A_T), Y(Y_SUP), 2 * A_T * s, (Y_INF - Y_SUP) * s, 5 * s);
    g.fillStyle = PAL.suave; g.font = '11px system-ui, sans-serif'; g.textAlign = 'left'; g.fillText('derecha del paciente', 8, 17); g.textAlign = 'right'; g.fillText('izquierda del paciente', w - 8, 17);
  } else {
    g.lineWidth = 1; g.strokeStyle = PAL.suave;
    const anillo = (cx, y, a, b) => { g.beginPath(); for (let k = 0; k <= 40; k++) { const q = k / 40 * 2 * Math.PI, p = P([cx + a * Math.cos(q), y, b * Math.sin(q)]); k ? g.lineTo(p[0], p[1]) : g.moveTo(p[0], p[1]); } g.stroke(); };
    for (let y = Y_SUP; y <= Y_INF + 0.1; y += 3.9) anillo(XM, y, A_T, B_T);
    for (let k = 0; k < 12; k++) { const q = k / 12 * 2 * Math.PI, a = P([XM + A_T * Math.cos(q), Y_SUP, B_T * Math.sin(q)]), b = P([XM + A_T * Math.cos(q), Y_INF, B_T * Math.sin(q)]); g.beginPath(); g.moveTo(a[0], a[1]); g.lineTo(b[0], b[1]); g.stroke(); }
    for (let la = -60; la <= 60; la += 30) { const r = 7.5 * Math.cos(la * Math.PI / 180); anillo(XM, -27.5 - 7.5 * Math.sin(la * Math.PI / 180), r, r); }
  }
  const col = [PAL.l1, PAL.l2, PAL.l3];
  [[0, 1], [0, 2], [1, 2]].forEach(([i, j], k) => { const a = P(pos[i]), b = P(pos[j]); g.strokeStyle = col[k]; g.lineWidth = 3; g.beginPath(); g.moveTo(a[0], a[1]); g.lineTo(b[0], b[1]); g.stroke(); });
  const c0 = P([0, 0, 0]), viva = st.corriendo && st.modo !== 4;
  if (viva) { g.strokeStyle = PAL.vec; g.globalAlpha = 0.45; g.lineWidth = 1.6; g.beginPath(); for (let i = 0; i < lat.N; i += 2) { const p = P([lat.h[0][i] * CM, lat.h[1][i] * CM, lat.h[2][i] * CM]); i ? g.lineTo(p[0], p[1]) : g.moveTo(p[0], p[1]); } g.stroke(); g.globalAlpha = 1; }
  const i = Math.min(lat.N - 1, Math.floor(ms)), H = viva ? [lat.h[0][i], lat.h[1][i], lat.h[2][i]] : [0, 0, 0], tip = P(H.map(x => x * CM));
  g.strokeStyle = PAL.vec; g.fillStyle = PAL.vec; g.lineWidth = 4; g.lineCap = 'round'; g.beginPath(); g.moveTo(c0[0], c0[1]); g.lineTo(tip[0], tip[1]); g.stroke();
  const an = Math.atan2(tip[1] - c0[1], tip[0] - c0[0]); if (Math.hypot(tip[0] - c0[0], tip[1] - c0[1]) > 7) { g.beginPath(); g.moveTo(tip[0], tip[1]); g.lineTo(tip[0] - 11 * Math.cos(an - 0.42), tip[1] - 11 * Math.sin(an - 0.42)); g.lineTo(tip[0] - 11 * Math.cos(an + 0.42), tip[1] - 11 * Math.sin(an + 0.42)); g.fill(); }
  g.fillStyle = '#d93a3a'; g.strokeStyle = PAL.borde; g.lineWidth = 2; g.beginPath(); g.arc(c0[0], c0[1], 5.5, 0, 7); g.fill(); g.stroke();
  pos.forEach((p, b) => { const q = P(p), mov = Math.hypot(p[0] - EST[b][0], p[1] - EST[b][1]) > 0.01;
    g.fillStyle = mov ? PAL['pzm-f'] : PAL['pz-f']; g.strokeStyle = PAL.borde; g.lineWidth = 3; g.beginPath(); g.arc(q[0], q[1], 14, 0, 7); g.fill(); g.stroke();
    g.fillStyle = mov ? PAL['pzm-t'] : PAL['pz-t']; g.font = '8px Pix, monospace'; g.textAlign = 'center'; g.textBaseline = 'middle'; g.fillText(NOM[b], q[0] + 0.5, q[1] + 1); g.textBaseline = 'alphabetic'; });
}
(() => {
  const cv = $('cvCuerpo'), cm = ev => { const r = cv.getBoundingClientRect(), s = Math.min(r.width / 66, r.height / 80); return [(ev.clientX - r.left - r.width / 2) / s - 3, (ev.clientY - r.top - r.height / 2) / s + 3]; };
  const aplicar = () => { st.L = pos.map(vecDer); st.libres = true; calcular(); reflejar(false); };
  cv.addEventListener('pointerdown', ev => { cv.setPointerCapture(ev.pointerId); if (vista === '3') { giro3 = [ev.clientX, ev.clientY, az, el]; return; }
    const [x, y] = cm(ev), d = pos.map(p => Math.hypot(p[0] - x, p[1] - y)), m = Math.min(...d); if (m < 6) { agarrada = d.indexOf(m); cv.style.cursor = 'grabbing'; } });
  cv.addEventListener('pointermove', ev => {
    if (giro3) { az = giro3[2] + (ev.clientX - giro3[0]) * 0.01; el = Math.max(-1.2, Math.min(1.2, giro3[3] + (ev.clientY - giro3[1]) * 0.01)); return; }
    if (agarrada === null) return; let [x, y] = cm(ev); x = Math.max(-34, Math.min(28, x)); y = Math.max(-34, Math.min(44, y)); pos[agarrada] = [x, y, prof(x, y)];
    aplicar(); if (modoCon === 'placa' && performance.now() - ultEnvio > 140) mandar(cmdLv()); });
  const soltar = () => { giro3 = null; cv.style.cursor = 'grab'; if (agarrada === null) return; const b = agarrada;
    for (const e of EST) if (Math.hypot(pos[b][0] - e[0], pos[b][1] - e[1]) < 2.2) pos[b] = e.slice();
    aplicar(); agarrada = null; mandar(cmdLv()); };
  cv.addEventListener('pointerup', soltar); cv.addEventListener('pointercancel', soltar);
})();

/* ====== MONITOR ====== */
const VENT = 4, FSM = 250, NB = VENT * FSM, buf = [new Float32Array(NB), new Float32Array(NB)]; let wp = 0, tMon = performance.now() - VENT * 1000 - 400;
function alimentar(ahora) {
  if (ahora - tMon > 6000) tMon = ahora - 6000;
  while (tMon + 4 <= ahora) { tMon += 4; let i = 0, a = 0, b = 0;
    if (st.corriendo) { i = Math.min(lat.N - 1, Math.floor(mod(faseMs() - (ahora - tMon), lat.N))); a = lat.mA[i]; b = lat.mB[i]; }
    buf[0][wp] = a; buf[1][wp] = b; wp = (wp + 1) % NB; }
}
function dibujarMonitor() {
  const { g, w, h } = lienzo($('cvMon')); g.fillStyle = PAL.pant; g.fillRect(0, 0, w, h);
  let pico = 0; for (let i = 0; i < NB; i++) pico = Math.max(pico, Math.abs(buf[0][i]), Math.abs(buf[1][i]), Math.abs(buf[1][i] - buf[0][i]));
  const lim = pico > 1.45 ? 3 : 1.5, paso = lim > 2 ? 1 : 0.5, fh = h / 3, col = [PAL.c1, PAL.c2, PAL.c3];
  $('monEsc').textContent = fmt(paso, 1) + ' mV POR LINEA';
  g.strokeStyle = PAL.reja; g.lineWidth = 1;
  for (let x = 0; x <= VENT * 5; x++) { const X = Math.round(x / (VENT * 5) * w) + 0.5; g.beginPath(); g.moveTo(X, 0); g.lineTo(X, h); g.stroke(); }
  for (let k = 0; k < 3; k++) { const y0 = fh * (k + 0.5), e = fh * 0.45 / lim;
    g.strokeStyle = PAL.reja; g.lineWidth = 1; for (let v = -lim; v <= lim + 1e-9; v += paso) { const Y = Math.round(y0 - v * e) + 0.5; g.globalAlpha = Math.abs(v) < 1e-9 ? 1 : 0.5; g.beginPath(); g.moveTo(0, Y); g.lineTo(w, Y); g.stroke(); }
    g.globalAlpha = 1; g.strokeStyle = col[k]; g.lineWidth = 2; g.beginPath(); let ab = false;
    for (let i = 0; i < NB; i++) { if (mod(wp - 1 - i, NB) > NB - 14) { ab = false; continue; } const v = k === 0 ? buf[0][i] : k === 1 ? buf[1][i] : buf[1][i] - buf[0][i], X = i / NB * w, Y = y0 - v * e; ab ? g.lineTo(X, Y) : g.moveTo(X, Y); ab = true; }
    g.stroke(); g.fillStyle = col[k]; g.font = '9px Pix, monospace'; g.textAlign = 'left'; g.fillText(['DI', 'DII', 'DIII'][k], 8, fh * k + 17); }
}

/* ====== INTERFAZ ====== */
let dicho = '', meta = '', nLet = 0;
function decir(t) { if (t !== meta) { meta = t; nLet = matchMedia('(prefers-reduced-motion: reduce)').matches ? t.length : 0; } }
function reflejar(mover = true) {
  pintarCon();
  const cal = st.modo === 4, a = lat.mA[lat.idxR], b = lat.mB[lat.idxR], ejeV = st.eje === null ? EJE_BASE : st.eje;
  document.querySelectorAll('#escs .b').forEach((x, i) => { const on = st.esc === i && !st.libres && st.eje === ESC[i].eje; x.classList.toggle('on', on); x.setAttribute('aria-pressed', on); });
  document.querySelectorAll('#ritmos .b').forEach((x, i) => { x.classList.toggle('on', st.modo === i); x.setAttribute('aria-pressed', st.modo === i); });
  if (document.activeElement !== $('eje')) $('eje').value = Math.round(ejeV / 5) * 5; $('ejeTxt').textContent = sg(ejeV, 0) + '°' + (st.espejo ? ' (EN ESPEJO)' : '');
  $('run').textContent = st.corriendo ? 'DETENER LA SALIDA' : 'INICIAR LA SALIDA';
  if (mover && !st.libres && agarrada === null) pos = ESC[st.esc].c.map(c => EST[c].slice());
  $('pista').textContent = vista === '3' ? 'Arrastra para girar el modelo. Para mover las pinzas, vuelve a la vista de frente.' : 'Arrastra una pinza para mover ese electrodo. Si la sueltas cerca de su lugar, se acomoda sola.';
  $('lect').innerHTML = !st.corriendo ? '<div class="t1"><b>SALIDA</b>DETENIDA</div><div class="t2"><b>DI Y DII</b>0 mV</div><div class="t3"><b>DIII</b>0 mV</div>'
    : cal ? '<div class="t1"><b>DI</b>1 mV</div><div class="t2"><b>DII</b>1 mV</div><div class="t3"><b>DIII</b>0 mV</div>'
    : [['R EN DI', a], ['R EN DII', b], ['R EN DIII', b - a]].map((x, i) => `<div class="t${i + 1}"><b>${x[0]}</b>${sg(x[1])} mV</div>`).join('');
  const v = x => fmt(1.55 + x * 1.101, 2) + ' V';
  $('osc').innerHTML = `<div><span>LINEA DE REPOSO</span><b>1,55 V</b></div><div><span>${cal ? 'ESCALON EN CH1' : 'PICO R EN CH1'}</span><b>${st.corriendo ? v(cal ? 1 : a) : '1,55 V'}</b></div><div><span>${cal ? 'ESCALON EN CH2' : 'PICO R EN CH2'}</span><b>${st.corriendo ? v(cal ? 1 : b) : '1,55 V'}</b></div>`;
  decir(!st.corriendo ? 'Salida detenida. Los dos bornes quedan en la línea de reposo.'
    : cal ? 'Calibración: un escalón de 1 mV en DI y en DII, una vez por segundo. DIII queda en cero.'
    : st.libres ? 'Moviste una pinza. La placa recalculó lo que ve cada electrodo en su nueva posición.' + (st.escala < 1 ? ` La señal se redujo al ${fmt(st.escala * 100, 0)} % para que quepa en la salida.` : '')
    : st.eje !== ESC[st.esc].eje ? `Giraste el corazón a ${sg(ejeV, 0)}°. Las tres derivaciones cambian juntas.`
    : ESC[st.esc].t);
}
$('escs').innerHTML = ESC.map((e, i) => `<button class="b" data-e="${i}"><i>${i}</i>${e.n}</button>`).join('');
$('ritmos').innerHTML = ['40', '60', '120', '180', 'CAL'].map((t, i) => `<button class="b" data-r="${i}" title="${i < 4 ? t + ' latidos por minuto' : 'Calibración de 1 mV'}">${t}</button>`).join('');
$('escs').addEventListener('click', ev => { const b = ev.target.closest('.b'); if (b) mandar(b.dataset.e); });
$('ritmos').addEventListener('click', ev => { const b = ev.target.closest('.b'); if (b) mandar('ritmo ' + b.dataset.r); });
function girar(grados, final) { st.eje = grados; if (st.modo === 4) st.modo = 1; calcular(); reflejar(); if (final || modoCon === 'demo' || performance.now() - ultEnvio > 140) mandar('eje ' + grados); }
$('eje').addEventListener('input', ev => girar(+ev.target.value, false));
$('eje').addEventListener('change', ev => girar(+ev.target.value, true));
$('run').addEventListener('click', () => mandar('run ' + (st.corriendo ? 0 : 1)));
$('reponer').addEventListener('click', () => mandar(String(st.esc)));
$('vF').addEventListener('click', () => { vista = 'f'; $('vF').classList.add('on'); $('v3').classList.remove('on'); reflejar(); });
$('v3').addEventListener('click', () => { vista = '3'; $('v3').classList.add('on'); $('vF').classList.remove('on'); reflejar(); });
document.addEventListener('keydown', ev => {
  if (ev.ctrlKey || ev.metaKey || ev.altKey || /^(INPUT|BUTTON|TEXTAREA|SELECT)$/.test(ev.target.tagName)) return;
  const k = ev.key, ejeV = Math.round(st.eje === null ? EJE_BASE : st.eje);
  if (/^[0-5]$/.test(k)) mandar(k);
  else if (k === 'ArrowLeft' || k === 'ArrowRight') girar(mod(ejeV + (k === 'ArrowRight' ? 15 : -15) + 180, 360) - 180, true);
  else if (k === 'ArrowUp' || k === 'ArrowDown') mandar('ritmo ' + Math.max(0, Math.min(3, (st.modo > 3 ? 1 : st.modo) + (k === 'ArrowUp' ? 1 : -1))));
  else if (k === 'c' || k === 'C') mandar('ritmo 4');
  else if (k === ' ') mandar('run ' + (st.corriendo ? 0 : 1));
  else return;
  ev.preventDefault();
});

/* ====== FICHA, LOGO Y TEMA ====== */
$('nomProy').textContent = FICHA.proyecto;
$('datos').innerHTML = [['UNIVERSIDAD', FICHA.universidad], ['CARRERA', FICHA.carrera], ['CURSO', FICHA.curso], ['INTEGRANTES', FICHA.integrantes]].map(([k, v]) => `<div><dt>${k}</dt><dd>${v}</dd></div>`).join('');
function ponerLogo(src) { if (!src) return; const im = new Image(); im.alt = 'Logo de ' + FICHA.universidad; im.src = src; $('logo').className = 'logo con'; $('logo').replaceChildren(im); }
ponerLogo(FICHA.logo || leer('ecg-logo'));
$('logo').addEventListener('click', () => $('logoArch').click());
$('logoArch').addEventListener('change', ev => {
  const f = ev.target.files[0]; if (!f) return; const lector = new FileReader();
  lector.onload = () => { const im = new Image(); im.onload = () => {             // se achica antes de guardarla en este navegador
    const e = Math.min(1, 240 / Math.max(im.width, im.height)), c = document.createElement('canvas'); c.width = Math.max(1, Math.round(im.width * e)); c.height = Math.max(1, Math.round(im.height * e));
    c.getContext('2d').drawImage(im, 0, 0, c.width, c.height); const u = c.toDataURL('image/png'); guardar('ecg-logo', u); ponerLogo(u); }; im.src = lector.result; };
  lector.readAsDataURL(f);
});
function tema(t) { document.documentElement.dataset.tema = t; $('tema').textContent = 'TEMA: ' + (t === 'rayo' ? 'RAYO' : 'CLINICA'); paleta(); guardar('ecg-tema', t); }
$('tema').addEventListener('click', () => tema(document.documentElement.dataset.tema === 'rayo' ? 'clinica' : 'rayo'));
tema(leer('ecg-tema') === 'rayo' ? 'rayo' : 'clinica');

/* ====== BUCLE ====== */
function cuadro() {
  const ahora = performance.now(), ms = faseMs();
  alimentar(ahora); dibujarMonitor(); dibujarCuerpo(ms);
  const d = ms - lat.idxR; $('cruz').classList.toggle('late', st.corriendo && st.modo !== 4 && d >= 0 && d < 90);
  if (nLet < meta.length) nLet = Math.min(meta.length, nLet + 2);
  const t = meta.slice(0, nLet); if (t !== dicho) { dicho = t; $('dial').textContent = t; }
  requestAnimationFrame(cuadro);
}
calcular(); reflejar();
(async () => {                                    // ¿hay una placa detrás de esta página?
  try { await pedir('/api/estado'); } catch (e) { modoCon = 'demo'; }
  reflejar();
  setInterval(() => { if (modoCon !== 'demo') cola = cola.then(() => pedir('/api/estado', true)).catch(() => { modoCon = 'caida'; pintarCon(); }); }, 1500);
})();
requestAnimationFrame(cuadro);
window.panel = { st, mandar, estado: () => modoCon, tablas: () => lat, pos: () => pos, fase: () => faseMs() };
})();
</script>
</body>
</html>
)PAGINA";
