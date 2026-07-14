#ifndef WEB_PAGE_H
#define WEB_PAGE_H

#include <Arduino.h>

// Self-contained, mobile-first dashboard + captive-portal setup page.
// No external fonts/CSS/JS (works with no internet in AP mode). Served for
// both "/" and captive-portal redirects; the JS decides which tab to show.
static const char INDEX_HTML[] PROGMEM = R"WCPAGE(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<title>Word Clock</title>
<style>
:root{
  --bg:#0d1017; --card:#161b26; --card2:#1e2534; --line:#2a3346;
  --txt:#e7ecf5; --dim:#8a97ad; --accent:#f6a821; --accent2:#3aa0ff;
  --ok:#3ad07a; --bad:#ff5d5d; --r:14px;
}
*{box-sizing:border-box}
html,body{margin:0;background:var(--bg);color:var(--txt);
  font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
  -webkit-font-smoothing:antialiased}
.wrap{max-width:640px;margin:0 auto;padding:18px 16px 60px}
header{display:flex;align-items:center;gap:12px;margin:6px 0 16px}
.logo{width:38px;height:38px;border-radius:10px;flex:0 0 auto;
  background:linear-gradient(135deg,var(--accent),#ff7a59);
  display:grid;place-items:center;font-weight:800;color:#1a1200}
h1{font-size:19px;margin:0;font-weight:700;letter-spacing:.2px}
.sub{font-size:12px;color:var(--dim);margin-top:2px}
.chip{margin-left:auto;font-size:12px;padding:6px 10px;border-radius:999px;
  background:var(--card2);border:1px solid var(--line);white-space:nowrap}
.chip b{color:var(--txt)}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:6px;
  vertical-align:middle;background:var(--dim)}
.dot.ok{background:var(--ok)} .dot.bad{background:var(--bad)}
.tabs{display:flex;gap:6px;background:var(--card);padding:6px;border-radius:var(--r);
  border:1px solid var(--line);margin-bottom:16px}
.tab{flex:1;text-align:center;padding:10px;border-radius:10px;font-size:14px;
  color:var(--dim);cursor:pointer;user-select:none}
.tab.on{background:var(--card2);color:var(--txt);font-weight:600}
.card{background:var(--card);border:1px solid var(--line);border-radius:var(--r);
  padding:16px;margin-bottom:14px}
.card h2{font-size:13px;text-transform:uppercase;letter-spacing:.08em;color:var(--dim);
  margin:0 0 14px}
.row{display:flex;align-items:center;justify-content:space-between;gap:12px;margin:12px 0}
.row label{font-size:14px}
.row .val{font-size:13px;color:var(--dim);min-width:52px;text-align:right}
input[type=range]{-webkit-appearance:none;width:100%;height:6px;border-radius:5px;
  background:var(--card2);outline:none}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:22px;height:22px;
  border-radius:50%;background:var(--accent);cursor:pointer;border:3px solid #241a06}
select,input[type=text],input[type=password],input[type=number]{width:100%;padding:11px 12px;
  border-radius:10px;border:1px solid var(--line);background:var(--card2);color:var(--txt);
  font-size:15px;outline:none}
input[type=color]{width:46px;height:34px;padding:0;border:1px solid var(--line);
  border-radius:8px;background:var(--card2)}
.field{margin:12px 0}
.field>span{display:block;font-size:13px;color:var(--dim);margin-bottom:6px}
.btn{display:block;width:100%;padding:13px;border:0;border-radius:11px;font-size:15px;
  font-weight:600;color:#1a1200;background:var(--accent);cursor:pointer;margin-top:6px}
.btn.sec{background:var(--card2);color:var(--txt);border:1px solid var(--line)}
.btn.bad{background:#3a1c1c;color:#ff9a9a;border:1px solid #5a2a2a}
.btn.row3{display:inline-block;width:auto;flex:1;padding:10px;margin:0}
.btnrow{display:flex;gap:8px}
.hide{display:none!important}
.nets{max-height:210px;overflow-y:auto;margin-top:6px}
.net{display:flex;align-items:center;gap:10px;padding:11px 12px;border-radius:10px;
  background:var(--card2);border:1px solid var(--line);margin-bottom:6px;cursor:pointer}
.net b{font-weight:600;font-size:14px} .net .rssi{margin-left:auto;font-size:12px;color:var(--dim)}
.lock{font-size:12px;color:var(--dim)}
.note{font-size:12px;color:var(--dim);margin-top:8px;line-height:1.5}
.toast{position:fixed;left:50%;bottom:22px;transform:translateX(-50%);background:var(--card2);
  border:1px solid var(--line);color:var(--txt);padding:11px 16px;border-radius:10px;
  font-size:14px;box-shadow:0 8px 30px rgba(0,0,0,.5);opacity:0;transition:.25s;pointer-events:none}
.toast.show{opacity:1}
.bar{height:8px;border-radius:5px;background:var(--card2);overflow:hidden;margin-top:8px}
.bar>i{display:block;height:100%;width:0;background:linear-gradient(90deg,var(--ok),var(--accent))}
.meter{font-size:12px;color:var(--dim);margin-top:6px}
</style>
</head>
<body>
<div class="wrap">
  <header>
    <div class="logo">W</div>
    <div>
      <h1>Word Clock</h1>
      <div class="sub" id="sub">connecting…</div>
    </div>
    <div class="chip" id="chip"><span class="dot" id="dot"></span><span id="chiptxt">…</span></div>
  </header>

  <div class="tabs">
    <div class="tab on" data-tab="clock">Clock</div>
    <div class="tab" data-tab="wifi">Wi-Fi</div>
    <div class="tab" data-tab="system">System</div>
  </div>

  <!-- ============ CLOCK ============ -->
  <section id="tab-clock">
    <div class="card">
      <h2>Brightness &amp; Power</h2>
      <div class="row">
        <label>Brightness</label>
        <input type="range" id="brightness" min="0" max="255" step="1" style="flex:1">
        <span class="val" id="brightnessV">–</span>
      </div>
      <div class="row">
        <label>Current budget</label>
        <input type="range" id="budget" min="200" max="4000" step="50" style="flex:1">
        <span class="val" id="budgetV">–</span>
      </div>
      <div class="meter">Est. draw now: <b id="estMa">–</b> mA &middot; effective brightness <b id="effB">–</b></div>
      <div class="bar"><i id="estBar"></i></div>
      <div class="note">The firmware never exceeds the current budget — brightness is scaled down automatically if a scene would draw too much. Raise the budget only to match your power supply.</div>
    </div>

    <div class="card">
      <h2>Color</h2>
      <div class="field">
        <span>Mode</span>
        <select id="colorMode">
          <option value="0">Random — each word a color</option>
          <option value="1">Single color</option>
          <option value="2">Gradient</option>
        </select>
      </div>
      <div class="field" id="singleWrap">
        <span>Single color</span>
        <input type="color" id="singleColor">
      </div>
      <div id="gradWrap">
        <div class="field">
          <span>Gradient style</span>
          <select id="gradVariant">
            <option value="0">Per-word (static rainbow)</option>
            <option value="1">Per-word (slow cycle)</option>
            <option value="2">Whole-face drifting rainbow</option>
          </select>
        </div>
        <div class="row">
          <label>Cycle speed</label>
          <input type="range" id="gradSpeed" min="0" max="255" step="1" style="flex:1">
          <span class="val" id="gradSpeedV">–</span>
        </div>
      </div>
    </div>

    <div class="card">
      <h2>On-the-hour animation</h2>
      <div class="row"><label>Enabled</label>
        <input type="checkbox" id="animEnabled" style="width:22px;height:22px"></div>
      <div class="field">
        <span>Effect</span>
        <select id="animId">
          <option value="0">Sparkle</option>
          <option value="1">Sweep</option>
          <option value="2">Rainbow</option>
          <option value="255">Random each hour</option>
        </select>
      </div>
      <button class="btn sec" id="previewAnim">Preview now</button>
    </div>

    <div class="card">
      <h2>Time zone</h2>
      <div class="field">
        <select id="tz">
          <option value="UTC0">UTC</option>
          <option value="EST5EDT,M3.2.0,M11.1.0">US Eastern</option>
          <option value="CST6CDT,M3.2.0,M11.1.0">US Central</option>
          <option value="MST7MDT,M3.2.0,M11.1.0">US Mountain</option>
          <option value="MST7">US Arizona (no DST)</option>
          <option value="PST8PDT,M3.2.0,M11.1.0">US Pacific</option>
          <option value="AKST9AKDT,M3.2.0,M11.1.0">US Alaska</option>
          <option value="HST10">US Hawaii</option>
          <option value="GMT0BST,M3.5.0/1,M10.5.0">UK / Ireland</option>
          <option value="CET-1CEST,M3.5.0,M10.5.0/3">Central Europe</option>
          <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Eastern Europe</option>
          <option value="IST-5:30">India</option>
          <option value="CST-8">China</option>
          <option value="JST-9">Japan / Korea</option>
          <option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Sydney</option>
        </select>
      </div>
      <div class="note">DST is applied automatically from the zone rules.</div>
    </div>

    <button class="btn" id="saveClock">Save clock settings</button>

    <div class="card" style="margin-top:14px">
      <h2>Diagnostics — verify the letter map</h2>
      <div class="field">
        <span>Word</span>
        <select id="testWord"></select>
      </div>
      <div class="btnrow">
        <button class="btn sec btn3" id="testShow">Show</button>
        <button class="btn sec btn3" id="testCycle">Cycle all</button>
        <button class="btn sec btn3" id="testAll">All on</button>
      </div>
      <button class="btn sec" id="testStop" style="margin-top:8px">Stop test</button>
      <div class="note">Use this against your printed template to confirm each word lights the right letters.</div>
    </div>
  </section>

  <!-- ============ WIFI ============ -->
  <section id="tab-wifi" class="hide">
    <div class="card">
      <h2>Wi-Fi setup</h2>
      <button class="btn sec" id="scan">Scan for networks</button>
      <div class="nets" id="nets"></div>
      <div class="field"><span>Network (SSID)</span><input type="text" id="ssid" autocomplete="off"></div>
      <div class="field"><span>Password</span>
        <input type="password" id="pass" autocomplete="off">
        <label class="note"><input type="checkbox" id="showpass"> show password</label>
      </div>
      <button class="btn" id="connect">Connect &amp; save</button>
      <div class="note">The clock saves these and reboots to join your network. If it can't connect, this setup portal reappears.</div>
    </div>
  </section>

  <!-- ============ SYSTEM ============ -->
  <section id="tab-system" class="hide">
    <div class="card">
      <h2>Device</h2>
      <div class="field"><span>Hostname (.local)</span><input type="text" id="hostname" autocomplete="off"></div>
      <button class="btn sec" id="saveHost">Save hostname</button>
      <div class="note" id="devinfo"></div>
    </div>
    <div class="card">
      <h2>Firmware update (OTA)</h2>
      <div class="field"><span>Firmware .bin</span><input type="file" id="fw" accept=".bin"></div>
      <button class="btn" id="upload">Upload &amp; flash</button>
      <div class="bar hide" id="otaWrap"><i id="otaBar"></i></div>
    </div>
    <div class="card">
      <h2>Danger zone</h2>
      <button class="btn bad" id="reset">Factory reset (clear Wi-Fi &amp; settings)</button>
    </div>
  </section>
</div>
<div class="toast" id="toast"></div>

<script>
const $=id=>document.getElementById(id);
const j=(u,o)=>fetch(u,o).then(r=>r.json());
let toastT;
function toast(m){const t=$('toast');t.textContent=m;t.classList.add('show');
  clearTimeout(toastT);toastT=setTimeout(()=>t.classList.remove('show'),2600);}

// tabs
document.querySelectorAll('.tab').forEach(t=>t.onclick=()=>{
  document.querySelectorAll('.tab').forEach(x=>x.classList.remove('on'));
  t.classList.add('on');
  ['clock','wifi','system'].forEach(n=>$('tab-'+n).classList.toggle('hide',n!==t.dataset.tab));
});
function showTab(n){document.querySelector('.tab[data-tab="'+n+'"]').click();}

function hex(n){return '#'+(n&0xFFFFFF).toString(16).padStart(6,'0');}
function toColorInt(h){return parseInt(h.slice(1),16)&0xFFFFFF;}

// ---- load settings ----
function pct(v){return Math.round(v/255*100)+'%';}
function syncColorUI(){
  const m=+$('colorMode').value;
  $('singleWrap').classList.toggle('hide',m!==1);
  $('gradWrap').classList.toggle('hide',m!==2);
}
async function loadSettings(){
  const s=await j('/api/settings');
  $('brightness').value=s.brightness; $('brightnessV').textContent=pct(s.brightness);
  $('budget').max=s.budgetMax||4000;
  $('budget').value=s.budgetMa; $('budgetV').textContent=s.budgetMa+' mA';
  $('colorMode').value=s.colorMode;
  $('singleColor').value=hex(s.singleColor);
  $('gradVariant').value=s.gradientVariant;
  $('gradSpeed').value=s.gradientSpeed; $('gradSpeedV').textContent=s.gradientSpeed;
  $('animEnabled').checked=!!s.animationEnabled;
  $('animId').value=s.animationId;
  $('tz').value=s.tz;
  $('hostname').value=s.hostname;
  $('ssid').value=s.ssid||'';
  syncColorUI();
  // word list for diagnostics
  const sel=$('testWord'); sel.innerHTML='';
  (s.words||[]).forEach((w,i)=>{const o=document.createElement('option');o.value=i;o.textContent=w;sel.appendChild(o);});
}
async function loadStatus(){
  try{
    const s=await j('/api/status');
    const ok=s.connected;
    $('dot').className='dot '+(ok?'ok':(s.portal?'bad':''));
    $('chiptxt').innerHTML=ok?('<b>'+s.time+'</b>'):(s.portal?'Setup mode':'offline');
    $('sub').textContent=ok?(s.ssid+' · '+s.ip):(s.portal?'Connect to configure Wi-Fi':'no network');
    $('estMa').textContent=s.estMa; $('effB').textContent=s.effB;
    $('estBar').style.width=Math.min(100,Math.round(s.estMa/s.budgetMa*100))+'%';
    $('devinfo').innerHTML='IP '+(s.ip||'—')+' · RSSI '+(s.rssi||'—')+' dBm · free heap '+Math.round((s.heap||0)/1024)+' KB · time '+(s.timeValid?'synced':'waiting for NTP');
    if(s.portal && !window._defaulted){window._defaulted=1;showTab('wifi');}
  }catch(e){}
}

// ---- live control bindings ----
$('brightness').oninput=e=>{$('brightnessV').textContent=pct(e.target.value);
  post({brightness:+e.target.value},true);};
$('budget').oninput=e=>{$('budgetV').textContent=e.target.value+' mA';
  post({budgetMa:+e.target.value},true);};
$('gradSpeed').oninput=e=>{$('gradSpeedV').textContent=e.target.value;
  post({gradientSpeed:+e.target.value},true);};
$('colorMode').onchange=()=>{syncColorUI();post({colorMode:+$('colorMode').value},true);};
$('singleColor').oninput=()=>post({singleColor:toColorInt($('singleColor').value)},true);
$('gradVariant').onchange=()=>post({gradientVariant:+$('gradVariant').value},true);
$('animEnabled').onchange=()=>post({animationEnabled:$('animEnabled').checked?1:0},true);
$('animId').onchange=()=>post({animationId:+$('animId').value},true);

let postT;
function post(obj,live){
  // debounce live tweaks so sliders don't spam the device
  if(live){clearTimeout(postT);postT=setTimeout(()=>send(obj),120);return;}
  return send(obj);
}
function send(obj){return fetch('/api/settings',{method:'POST',
  headers:{'Content-Type':'application/json'},body:JSON.stringify(obj)});}

$('saveClock').onclick=async()=>{
  await send({brightness:+$('brightness').value,budgetMa:+$('budget').value,
    colorMode:+$('colorMode').value,singleColor:toColorInt($('singleColor').value),
    gradientVariant:+$('gradVariant').value,gradientSpeed:+$('gradSpeed').value,
    animationEnabled:$('animEnabled').checked?1:0,animationId:+$('animId').value,
    tz:$('tz').value,save:1});
  toast('Saved');
};
$('previewAnim').onclick=()=>{fetch('/api/preview',{method:'POST'});toast('Playing…');};
$('tz').onchange=()=>post({tz:$('tz').value},true);

// diagnostics
function testReq(o){return fetch('/api/test',{method:'POST',
  headers:{'Content-Type':'application/json'},body:JSON.stringify(o)});}
$('testShow').onclick=()=>testReq({mode:'single',index:+$('testWord').value});
$('testCycle').onclick=()=>testReq({mode:'cycle'});
$('testAll').onclick=()=>testReq({mode:'all'});
$('testStop').onclick=()=>testReq({mode:'off'});

// wifi
$('showpass').onchange=e=>$('pass').type=e.target.checked?'text':'password';
$('scan').onclick=async()=>{
  $('scan').textContent='Scanning…';
  let tries=0;
  const poll=async()=>{
    const r=await j('/api/scan');
    if(r.scanning && tries++<12){setTimeout(poll,900);return;}
    $('scan').textContent='Scan for networks';
    const box=$('nets');box.innerHTML='';
    (r.networks||[]).forEach(n=>{
      const d=document.createElement('div');d.className='net';
      d.innerHTML='<b>'+n.ssid+'</b><span class="lock">'+(n.secure?'🔒':'')+
        '</span><span class="rssi">'+n.rssi+' dBm</span>';
      d.onclick=()=>{$('ssid').value=n.ssid;$('pass').focus();};
      box.appendChild(d);
    });
    if(!(r.networks||[]).length)box.innerHTML='<div class="note">No networks found.</div>';
  };poll();
};
$('connect').onclick=async()=>{
  const ssid=$('ssid').value.trim(); if(!ssid){toast('Enter a network');return;}
  await fetch('/api/wifi',{method:'POST',headers:{'Content-Type':'application/json'},
    body:JSON.stringify({ssid,pass:$('pass').value})});
  toast('Saved — rebooting to connect…');
};

// system
$('saveHost').onclick=async()=>{await send({hostname:$('hostname').value.trim(),save:1});
  toast('Saved — reboot to apply hostname');};
$('reset').onclick=async()=>{if(!confirm('Erase Wi-Fi and all settings?'))return;
  await fetch('/api/reset',{method:'POST'});toast('Reset — rebooting…');};
$('upload').onclick=()=>{
  const f=$('fw').files[0]; if(!f){toast('Choose a .bin file');return;}
  const fd=new FormData();fd.append('firmware',f,f.name);
  const xhr=new XMLHttpRequest();xhr.open('POST','/api/ota');
  $('otaWrap').classList.remove('hide');
  xhr.upload.onprogress=e=>{if(e.lengthComputable)$('otaBar').style.width=Math.round(e.loaded/e.total*100)+'%';};
  xhr.onload=()=>{toast(xhr.status==200?'Flashed — rebooting…':'Update failed');};
  xhr.onerror=()=>toast('Upload error');
  xhr.send(fd);
};

loadSettings();loadStatus();setInterval(loadStatus,3000);
</script>
</body></html>)WCPAGE";

#endif // WEB_PAGE_H
