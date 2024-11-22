const ip = "http://192.168.160.121/set_alarm"; // Replace with the actual IP address of your ESP32

let alarms = [];
const alarmList = document.getElementById("alarm-list");
const noAlarmsMessage = document.getElementById("no-alarms-message");

// Show the form to add an alarm
function showAddAlarmForm() {
  document.getElementById("add-alarm-form").classList.remove("hidden");
}

// Cancel the add alarm form
function cancelAddAlarm() {
  document.getElementById("add-alarm-form").classList.add("hidden");
}

// Confirm and save the alarm
function confirmAddAlarm() {
  const time = document.getElementById("alarm-time").value;
  const days = Array.from(
    document.querySelectorAll(".days-selector span.selected")
  ).map((span) => span.getAttribute("data-day"));
  const newAlarm = { time, days, id: Date.now(), enabled: true };

  alarms.push(newAlarm);
  updateAlarmList();
  document.getElementById("add-alarm-form").classList.add("hidden");
  sendAlarmToESP32(newAlarm);
}

// Update the alarm list displayed in the UI
function updateAlarmList() {
  alarmList.innerHTML = ""; // Clear the alarm list content
  if (alarms.length === 0) {
    noAlarmsMessage.style.display = "block";
  } else {
    noAlarmsMessage.style.display = "none";
    alarms.forEach((alarm) => {
      const alarmItem = document.createElement("div");
      alarmItem.className = "alarm-item";
      alarmItem.innerHTML = `
        <span>${alarm.time}</span>
        <span>${alarm.days.join(" ")}</span>
        <button onclick="toggleAlarm(${alarm.id})">${
        alarm.enabled ? "Desactivar" : "Activar"
      }</button>
        <button onclick="showDeleteAlarmForm(${alarm.id})">
          <i class='bx bx-trash'></i>
        </button>
      `;
      alarmList.appendChild(alarmItem);
    });
  }
}

// Send the alarm to the ESP32
function sendAlarmToESP32(alarmTime) {
  fetch('http://192.168.160.121/set_alarm', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/x-www-form-urlencoded'
    },
    body: `time=${encodeURIComponent(alarmTime.time)}` // Only send the time property
  })
    .then(response => response.text())
    .then(data => console.log(data))
    .catch(error => console.error('Error sending alarm:', error));
}

// Toggle alarm activation
function toggleAlarm(id) {
  const alarm = alarms.find((a) => a.id === id);
  if (alarm) {
    alarm.enabled = !alarm.enabled;
    updateAlarmList();
  }
}

// Show the delete alarm form
function showDeleteAlarmForm(id) {
  const confirmDelete = confirm("¿Seguro que quieres eliminar esta alarma?");
  if (confirmDelete) {
    alarms = alarms.filter((a) => a.id !== id);
    updateAlarmList();
  }
}
