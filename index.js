"use strict";
var output;
var ws;
var interval_ids = [0, 0, 0, 0, 0]; // for five timers

const CANCELLED = "CANCEL DONE";
const EXECUTED = "CALLBACK EXECUTED";
const INACTIVE = "NOT ACTIVE";
const ACTIVE = "ACTIVE";
const EXPIRING = "EXPIRING";

function init() {
    output = document.getElementById("output");

    let wsUri = 'ws://' + window.location.hostname+ ':7654';

    ws = new WebSocket(wsUri);
    ws.onopen = () => {
        writeToScreen('CONNECTED', output);
    };
    ws.onclose = () => writeToScreen('<p style="color: red;">DISCONNECTED</p>');
    ws.onmessage = onMessage;
    ws.onerror = msg => writeToScreen('<p style="color: red;">ERROR: ' + msg.data + '</p>');
}

function onMessage(ws_msg) {
    // Print reply message from server
    writeToScreen('<p style="color: blue;">RESPONSE: ' + ws_msg.data + '</p>');

    if (ws_msg.data.startsWith(CANCELLED)) {   // CANCELLED:1
        let id = ws_msg.data[CANCELLED.length + 1];
        document.getElementById('demo' + id).innerHTML = INACTIVE;
        document.getElementById('set' + id).disabled = false;
    } else if (ws_msg.data.startsWith(EXECUTED)) {
        let id = ws_msg.data[EXECUTED.length + 1];
        document.getElementById('demo' + id).innerHTML = EXECUTED;
        document.getElementById('set' + id).disabled = false;
    }
}

function writeToScreen(message) {
    let p = document.createElement('p');
    p.style.wordWrap = 'break-word';
    p.innerHTML = message;

    if (output.childNodes.length > 5) {
        output.removeChild(output.childNodes[0]);
    }
    output.appendChild(p);
}

function doSend(message) {
    writeToScreen("SENT: " + message);
    ws.send(message);
}

window.addEventListener('load', init, false);

function wsSendSet(i) {
    var seconds = document.getElementById('data_to_send' + i).value;
    doSend("SET:" + i + ":" + seconds);
    startCountDownDemo(parseInt(i), parseInt(seconds));
    document.getElementById('set' + i).disabled = true;
}

function wsSendCancel(i) {
    doSend("CANCEL:" + i + ":");    // CANCEL:<i>: msg must contains two ":"
}

function wsSendReset(i) {
    var seconds = document.getElementById('data_to_send' + i).value;
    doSend("RESET:" + i + ":" + seconds);
    if (interval_ids[parseInt(i)] !== 0) {
        clearInterval(interval_ids[parseInt(i)]);
    }
    startCountDownDemo(parseInt(i), seconds);
}

function startCountDownDemo(i, seconds) {
    var distance = seconds * 1000;
    const demo_id =  "demo" + i;
    document.getElementById(demo_id).innerHTML = ACTIVE;

    interval_ids[i] = setInterval(function () {
        // Countdown cancelled in backend
        if (document.getElementById(demo_id).innerHTML == INACTIVE
                || document.getElementById(demo_id).innerHTML == EXECUTED) {
            clearInterval(interval_ids[i]);
            return;
        }
        
        // Display the result in the element with id="demo"
        distance = distance - 100;
        document.getElementById(demo_id).innerHTML = distance + "ms";

        // If the count down is finished, write some text 
        if (distance < 0) {
            clearInterval(interval_ids[i]);
            document.getElementById(demo_id).innerHTML = EXPIRING;
        }
    }, 100);
}