async function loadState() {
    const response = await fetch("/api");
    const data = await response.json();
    document.getElementById("status").innerText =
        data.currentTemp;

    document.getElementById("textInput").placeholder = data.tempOff;
    document.getElementById("textInput2").placeholder = data.tempInterval;
}

async function setLed(state) {
    await fetch(state ? "/on" : "/off");
    loadState();
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

setInterval(loadState, 1000);
loadState();