// Firebase configuration - using your actual project values
const firebaseConfig = {
    apiKey: "AIzaSyA1n-8l6g4cNwVCLJHOpcrmQfBSD4CCNKY",
    authDomain: "wearablebraceletsystem.firebaseapp.com",
    databaseURL: "https://wearablebraceletsystem-default-rtdb.europe-west1.firebasedatabase.app",
    projectId: "wearablebraceletsystem",
    storageBucket: "wearablebraceletsystem.firebasestorage.app",
    messagingSenderId: "993813204044",
    appId: "1:993813204044:web:5a2836d50e06ce985038e9",
    measurementId: "G-BQLH5GEE1W"
  };
  
  // Initialize Firebase
  firebase.initializeApp(firebaseConfig);
  const database = firebase.database();
  
  // Update current time
  function updateCurrentTime() {
      const now = new Date();
      document.getElementById('current-time').textContent = now.toLocaleString();
  }
  setInterval(updateCurrentTime, 1000);
  updateCurrentTime();
  
  // Get status badge class
  function getStatusClass(status) {
      if (!status) return '';
      
      if (status.includes('DETECTED')) return 'status-detected';
      if (status.includes('LOST')) return 'status-lost';
      if (status.includes('VIBRATE')) return 'status-vibrate';
      if (status.includes('AUTHENTICATED')) return 'status-authenticated';
      return '';
  }
  
  // Get event row class
  function getEventRowClass(event) {
      if (!event) return '';
      
      if (event.includes('DETECTED')) return 'event-row-detected';
      if (event.includes('LOST')) return 'event-row-lost';
      if (event.includes('VIBRATE')) return 'event-row-vibrate';
      if (event.includes('AUTHENTICATED')) return 'event-row-authenticated';
      return '';
  }
  
  // Format RSSI as signal strength
  function formatRSSI(rssi) {
      if (!rssi) return 'N/A';
      
      if (rssi > -70) return `<span class="badge bg-success badge-rssi">Signal: Strong (${rssi} dBm)</span>`;
      if (rssi > -85) return `<span class="badge bg-warning text-dark badge-rssi">Signal: Medium (${rssi} dBm)</span>`;
      return `<span class="badge bg-danger badge-rssi">Signal: Weak (${rssi} dBm)</span>`;
  }
  
  // Create beacon card HTML
  function createBeaconCard(beaconId, data) {
      const statusClass = getStatusClass(data.status);
      const lastSeen = data.lastSeen || 'Unknown';
      const room = data.lastRoom || 'Unknown';
      const rssi = formatRSSI(data.rssi);
      
      return `
          <div class="col">
              <div class="card beacon-card ${statusClass}">
                  <div class="card-body">
                      <h5 class="card-title">${beaconId.substring(0, 15)}...</h5>
                      <p class="card-text">Status: <strong>${data.status || 'Unknown'}</strong></p>
                      <p class="card-text">Room: ${room}</p>
                      <p class="card-text">Last seen: ${lastSeen}</p>
                      ${rssi}
                  </div>
              </div>
          </div>
      `;
  }
  
  // Create room card HTML
  function createRoomCard(roomName, beacons) {
      const beaconCount = Object.keys(beacons || {}).length;
      const active = Object.values(beacons || {})
          .filter(b => b.status && (b.status.includes('DETECTED') || b.status.includes('VIBRATE')))
          .length;
      
      return `
          <div class="col">
              <div class="card room-card">
                  <div class="card-body">
                      <h5 class="card-title">${roomName}</h5>
                      <p class="card-text">Total beacons: ${beaconCount}</p>
                      <p class="card-text">Active beacons: ${active}</p>
                  </div>
              </div>
          </div>
      `;
  }
  
  // Listen for beacons data changes
  database.ref('/beacons').on('value', (snapshot) => {
      const beaconsContainer = document.getElementById('beacons-container');
      beaconsContainer.innerHTML = '';
      
      const beacons = snapshot.val() || {};
      
      if (Object.keys(beacons).length === 0) {
          beaconsContainer.innerHTML = '<div class="col-12 text-center">No beacons detected</div>';
          return;
      }
      
      Object.entries(beacons).forEach(([beaconId, data]) => {
          beaconsContainer.innerHTML += createBeaconCard(beaconId, data);
      });
  });
  
  // Listen for rooms data changes
  database.ref('/rooms').on('value', (snapshot) => {
      const roomsContainer = document.getElementById('rooms-container');
      roomsContainer.innerHTML = '';
      
      const rooms = snapshot.val() || {};
      
      if (Object.keys(rooms).length === 0) {
          roomsContainer.innerHTML = '<div class="col-12 text-center">No rooms configured</div>';
          return;
      }
      
      Object.entries(rooms).forEach(([roomName, data]) => {
          roomsContainer.innerHTML += createRoomCard(roomName, data.beacons);
      });
  });
  
  // Listen for events data changes (most recent 50)
  database.ref('/events').limitToLast(50).on('value', (snapshot) => {
      const eventsTable = document.getElementById('events-table');
      eventsTable.innerHTML = '';
      
      const events = snapshot.val() || {};
      
      if (Object.keys(events).length === 0) {
          eventsTable.innerHTML = '<tr><td colspan="4" class="text-center">No events recorded</td></tr>';
          return;
      }
      
      // Convert to array and sort by timestamp (newest first)
      const eventsArray = Object.entries(events).map(([key, event]) => ({
          id: key,
          ...event
      }));
      
      eventsArray.sort((a, b) => {
          const dateA = new Date(a.timestamp);
          const dateB = new Date(b.timestamp);
          return dateB - dateA;
      });
      
      // Generate table rows
      eventsArray.forEach(event => {
          const rowClass = getEventRowClass(event.event);
          eventsTable.innerHTML += `
              <tr class="${rowClass}">
                  <td>${event.timestamp}</td>
                  <td>${event.event}</td>
                  <td>${event.beacon || 'N/A'}</td>
                  <td>${event.room || 'N/A'}</td>
              </tr>
          `;
      });
  });
  