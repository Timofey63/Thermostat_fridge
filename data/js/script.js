let prevHistory = [];

async function loadState() {
    const response = await fetch("/api");
    const data = await response.json();
    
    const statusElement = document.getElementById("status");
    if (statusElement.innerText != data.currentTemp) {
        statusElement.innerText = data.currentTemp;
    }

    document.getElementById("textInput").placeholder = data.tempOff;
    document.getElementById("textInput2").placeholder = data.tempInterval;

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
        const fragment = document.createDocumentFragment();
        
        for (let i = history.length - 1; i >= 0; i--) {
            const p = document.createElement('p');
            p.textContent = history[i];
            fragment.appendChild(p);
        }
        
        container.appendChild(fragment);
    } else {
        container.innerHTML = '<p>No data</p>';
    }
}

async function sendText1() {
    const text1 = document.getElementById("textInput").value;
    await fetch(`/sendText?value1=${encodeURIComponent(text1)}`);
    console.log("Sent text1:", text1);
    document.getElementById("textInput").value = "";
}

async function sendText2() {
    const text2 = document.getElementById("textInput2").value;
    await fetch(`/sendText?value2=${encodeURIComponent(text2)}`);
    console.log("Sent text2:", text2);
    document.getElementById("textInput2").value = "";
}

setInterval(loadState, 10000);
loadState();