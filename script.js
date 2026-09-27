let port;
let writer;
let reader;

const connectBtn = document.getElementById('connectBtn');
const statusText = document.getElementById('status');

// USB Bağlantısını Başlat
connectBtn.addEventListener('click', async () => {
    try {
        port = await navigator.serial.requestPort();
        await port.open({ baudRate: 115200 });

        statusText.innerText = "Bağlandı!";
        statusText.style.color = "#28a745";
        connectBtn.style.display = "none";

        const textEncoder = new TextEncoderStream();
        textEncoder.readable.pipeTo(port.writable);
        writer = textEncoder.writable.getWriter();

        const textDecoder = new TextDecoderStream();
        port.readable.pipeTo(textDecoder.writable);
        reader = textDecoder.readable.getReader();

        await sendCommand("MOT:S");
        readLoop();
    } catch (error) {
        console.error("Bağlantı hatası:", error);
        statusText.innerText = "Bağlantı Başarısız!";
    }
});

// Arduinoya komut gönderen fonksiyon
async function sendCommand(cmd) {
    if (writer) {
        await writer.write(cmd + "\n");
        console.log("Gönderilen:", cmd);
    }
}

// --- ARAÇ KONTROLLERİ (FARE) ---
const setupMouseControl = (btnId, cmd) => {
    const btn = document.getElementById(btnId);
    // Tıklandığında harekete başla
    btn.addEventListener('mousedown', () => sendCommand(cmd));
    // Bırakıldığında veya fare buton dışına çıktığında dur
    btn.addEventListener('mouseup', () => sendCommand("MOT:S"));
    btn.addEventListener('mouseleave', () => sendCommand("MOT:S")); 
};

setupMouseControl('btnForward', 'MOT:F');
setupMouseControl('btnBackward', 'MOT:B');
setupMouseControl('btnLeft', 'MOT:L');
setupMouseControl('btnRight', 'MOT:R');
document.getElementById('btnStop').addEventListener('click', () => sendCommand("MOT:S"));

// --- ARAÇ KONTROLLERİ (KLAVYE W-A-S-D) ---
let currentKey = null;

document.addEventListener('keydown', (event) => {
    // Odak input/slider üzerindeyse klavye kontrolünü devredışı bırak
    if (document.activeElement.tagName === "INPUT") return;
    if (event.repeat) return; // Basılı tutulduğunda komut spamını engeller

    const key = event.key.toLowerCase();
    if (['w', 'a', 's', 'd'].includes(key)) {
        currentKey = key;
        if (key === 'w') sendCommand("MOT:F");
        if (key === 's') sendCommand("MOT:B");
        if (key === 'a') sendCommand("MOT:L");
        if (key === 'd') sendCommand("MOT:R");
    }
});

document.addEventListener('keyup', (event) => {
    const key = event.key.toLowerCase();
    if (key === currentKey) {
        sendCommand("MOT:S"); // Tuş bırakıldığında dur
        currentKey = null;
    }
});

// --- SERVO KONTROLLERİ ---
function updateServo(index, value) {
    document.getElementById(`srv${index}-val`).innerText = value;
    sendCommand(`SRV:${index}:${value}`);
}

// --- SENSÖR VERİ OKUMA DÖNGÜSÜ ---
async function readLoop() {
    let buffer = "";
    while (true) {
        const { value, done } = await reader.read();
        if (done) {
            reader.releaseLock();
            break;
        }
        
        buffer += value;
        let lines = buffer.split('\n');
        buffer = lines.pop(); // Son eleman tam satır olmayabilir, beklet

        for (let line of lines) {
            line = line.trim();
            if (line.startsWith("{") && line.endsWith("}")) {
                try {
                    const data = JSON.parse(line);
                    
                    if (data.T1 !== undefined) document.getElementById('t1-val').innerText = data.T1.toFixed(1);
                    if (data.T2 !== undefined) document.getElementById('t2-val').innerText = data.T2.toFixed(1);
                    if (data.T3 !== undefined) document.getElementById('t3-val').innerText = data.T3.toFixed(1);
                    if (data.AccelX !== undefined) document.getElementById('accx-val').innerText = data.AccelX.toFixed(2);
                    if (data.AccelY !== undefined) document.getElementById('accy-val').innerText = data.AccelY.toFixed(2);
                } catch (e) {
                    console.error("JSON Çözümleme Hatası:", e, line);
                }
            }
        }
    }
}