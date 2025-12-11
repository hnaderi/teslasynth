import Alpine from 'alpinejs'

window.Alpine = Alpine
Alpine.start()

import logoRaw from '../../logo.svg?raw';
const parser = new DOMParser();
const svgDoc = parser.parseFromString(logoRaw, "image/svg+xml");
const svgElement = svgDoc.documentElement;
svgElement.classList.add("svg-logo");

document.getElementById("logo").appendChild(svgElement);

// Example: fetch device status periodically
function fetchStatus() {
    fetch('/api/status')
        .then(res => res.json())
        .then(data => {
            document.getElementById('uptime').textContent = data.uptime + ' s'
            document.getElementById('temp').textContent = data.temperature + ' °C'
        })
        .catch(console.error)
}

fetchStatus()
setInterval(fetchStatus, 5000)

