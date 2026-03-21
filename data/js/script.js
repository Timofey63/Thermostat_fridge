let prevHistory = [];
let tempOff = -15;
let tempInterval = 2;

async function loadState() {
    const response = await fetch("/api");
    const data = await response.json();

    tempOff = data.tempOff;
    tempInterval = data.tempInterval;
    
    const statusElement = document.getElementById("status");
    if (statusElement.innerText != data.currentTemp) {
        statusElement.innerText = data.currentTemp;
    }

    document.getElementById("textInput").placeholder = data.tempOff;
    document.getElementById("textInput2").placeholder = data.tempInterval;
    document.getElementById("textInput3").placeholder = data.timeUpdate;
    document.getElementById("textInput4").placeholder = data.maxRuntime;

    const historyDiv = document.getElementById('historyList');
    if (historyDiv) {
        if (JSON.stringify(prevHistory) !== JSON.stringify(data.history)) {
            updateHistory(historyDiv, data.history);
            prevHistory = data.history || [];
        }
    }
}

function updateHistory(container, history) {
    container.innerHTML = '';
    
    if (history && history.length > 0) {
        const temps = history.map(t => parseFloat(t));
        const minTemp = Math.min(...temps);
        const maxTemp = Math.max(...temps);
        const range = maxTemp - minTemp || 1;

        const graphContainer = document.createElement('div');
        graphContainer.style.display = 'flex';
        graphContainer.style.alignItems = 'flex-end';
        graphContainer.style.gap = '2px';
        graphContainer.style.height = '200px';
        graphContainer.style.overflowX = 'auto';
        graphContainer.style.overflowY = 'hidden'; 
        graphContainer.style.padding = '10px 0';
        graphContainer.style.backgroundColor = '#ffffff';
        graphContainer.style.whiteSpace = 'nowrap';

        const totalWidth = history.length * 37; // 35px width + 2px gap
        graphContainer.style.minWidth = '100%';
        graphContainer.style.width = totalWidth + 'px';
        
        for (let i = history.length - 1; i >= 0; i--) {
            const tempValue = parseFloat(history[i]);
            
            const normalized = (tempValue - minTemp) / range;
            const height = 30 + normalized * 150;
            
            const barContainer = document.createElement('div');
            barContainer.style.display = 'flex';
            barContainer.style.flexDirection = 'column';
            barContainer.style.alignItems = 'center';
            barContainer.style.width = '35px';
            barContainer.style.margin = '0 2px';
            
            const bar = document.createElement('div');
            bar.style.height = height + 'px';
            bar.style.width = '30px';
            bar.style.backgroundColor = getColorForTemp(tempValue);
            bar.style.borderRadius = '3px 3px 0 0';
            
            const label = document.createElement('span');
            label.textContent = tempValue;
            label.style.fontSize = '10px';
            label.style.marginTop = '4px';
            label.style.fontWeight = 'bold';
            label.style.color = '#000000';
            
            barContainer.appendChild(bar);
            barContainer.appendChild(label);
            graphContainer.appendChild(barContainer);
        }
        
        container.appendChild(graphContainer);
        
    } else {
        container.innerHTML = '<p style="color: #666; text-align: center;">No temperature history</p>';
    }
}

function getColorForTemp(temp) {
    const tempOn = tempOff + tempInterval;
    
    if (temp < tempOff) return '#003d79';
    if (temp < tempOn) return '#36d972';
    if (temp < 0) return '#ff9933';
    return '#ff3333';
}

async function sendText1() {
    const text1 = document.getElementById("textInput").value;
    if (text1.trim() === '') return;
    await fetch(`/sendText?value1=${encodeURIComponent(text1)}`);
    console.log("Sent text1:", text1);
    document.getElementById("textInput").value = "";
}

async function sendText2() {
    const text2 = document.getElementById("textInput2").value;
    if (text2.trim() === '') return;
    await fetch(`/sendText?value2=${encodeURIComponent(text2)}`);
    console.log("Sent text2:", text2);
    document.getElementById("textInput2").value = "";
}

async function sendText3() {
    const value = document.getElementById("textInput3").value;
    if (value.trim() === '') return;
    await fetch(`/sendText?value3=${encodeURIComponent(value)}`);
    console.log("Sent time update:", value);
    document.getElementById("textInput3").value = "";
}

async function sendText4() {
    const value = document.getElementById("textInput4").value;
    if (value.trim() === '') return;
    await fetch(`/sendText?value4=${encodeURIComponent(value)}`);
    console.log("Sent max runtime:", value);
    document.getElementById("textInput4").value = "";
}

// Загружаем сразу и каждые 3 секунды
loadState();
setInterval(loadState, 3000);