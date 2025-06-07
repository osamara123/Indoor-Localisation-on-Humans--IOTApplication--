<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  
</head>
<body>

  <h1>IoT Contact‑Tracing & Occupancy‑Monitoring Platform</h1>

  <p><strong>Presented By:</strong> Omar Zainelabideen, Hassan Hassan, Osama Amer, Abdullah El Nahhas<br>
     <strong>Advisor:</strong> Professor Yasser Gadallah<br>
     <strong>Institution:</strong> The American University in Cairo</p>

  <nav>
    <h2>Table of Contents</h2>
    <ul>
      <li><a href="#introduction">Introduction</a></li>
      <li><a href="#objectives">Objectives</a></li>
      <li><a href="#use-cases">Use Cases</a></li>
      <li><a href="#key-features">Key Features</a></li>
      <li><a href="#system-architecture">System Architecture</a></li>
      <ul>
        <li><a href="#wearable-client">Wearable Client</a></li>
        <li><a href="#room-gateway">Room Gateway</a></li>
        <li><a href="#cloud-integration">Cloud Integration</a></li>
        <li><a href="#fallback-local-machine">Fallback: Local Machine</a></li>
      </ul>
      <li><a href="#comparative-table">Comparative Table</a></li>
      <li><a href="#workflow">Workflow</a></li>
      <li><a href="#privacy-security">Privacy & Security</a></li>
      <li><a href="#financial-analysis">Financial Analysis (EGP)</a></li>
      <li><a href="#differentiation">Differentiation</a></li>
      <li><a href="#references">References</a></li>
    </ul>
  </nav>

  <section id="introduction">
    <h2>Introduction</h2>
    <p>
      Modern organizations need a friction-free, privacy-aware way to know who was in which room and for how long—without cameras, swipe cards, or queues. Our platform combines continuous Bluetooth Low Energy (BLE) presence detection with randomized on-device fingerprint verification. A lightweight wearable (BLE peripheral) auto-attaches to room gateways (BLE centrals), which stream entry/exit timestamps to the cloud. Dwell time is computed server-side, and tamper/fail alerts occur in real time. The result: unobtrusive, low-cost, reliable room-level logging that respects privacy.
    </p>
  </section>

  <section id="objectives">
    <h2>Objectives</h2>
    <ul>
      <li>Seamless, high-fidelity room logging via zero-interaction BLE</li>
      <li>Active identity assurance with randomized fingerprint challenges</li>
      <li>Instant tamper/alert pathway on three failed scans</li>
      <li>Low power & low cost: commodity ESP32-C3 modules</li>
      <li>Actionable cloud analytics with dwell-time dashboards</li>
    </ul>
  </section>

  <section id="use-cases">
    <h2>Use Cases</h2>
    <h3>Medical Residency Compliance</h3>
    <p>
      <strong>Problem:</strong> Manual sign-in/out or fixed kiosks enable “buddy punching.”<br>
      <strong>Solution:</strong> Wearables auto-log ward rounds and randomly demand fingerprints; tamper alerts plus duration logs prove actual time on duty.
    </p>
    <h3>Camera-Free Security Facilities</h3>
    <p>
      <strong>Problem:</strong> Sensitive labs prohibit cameras but require real-time traceability.<br>
      <strong>Solution:</strong> BLE presence mesh provides room-level localization; fingerprint challenges block unauthorized device sharing.
    </p>
  </section>

  <section id="key-features">
    <h2>Key Features</h2>
    <table>
      <thead>
        <tr>
          <th>Feature</th>
          <th>Description</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>Multi-Room BLE Presence</td>
          <td>Peripheral advertises a secure BLE service; gateway centrals auto-connect sub-second.</td>
        </tr>
        <tr>
          <td>Periodic Fingerprint Challenge</td>
          <td>Gateway vibrates wearable at random 2–10 min intervals; successful scan resets counter.</td>
        </tr>
        <tr>
          <td>Three-Strike Tamper Alarm</td>
          <td>Three consecutive failures → triple buzz + ALERT event published.</td>
        </tr>
        <tr>
          <td>Cloud Dwell-Time Logging</td>
          <td>Gateways POST <code>{user_id, room_id, t_enter, t_exit}</code>; backend aggregates & exposes API/dashboard.</td>
        </tr>
        <tr>
          <td>Lightweight & Private</td>
          <td>No cameras/mics; biometric matching occurs locally on the wearable MCU.</td>
        </tr>
      </tbody>
    </table>
  </section>

  <section id="system-architecture">
    <h2>System Architecture</h2>
    <h3> Wearable Client</h3>
    <ul>
      <li><strong>MCU:</strong> ESP32-C3 Super Mini (NimBLE peripheral)</li>
      <li><strong>Sensor:</strong> Capacitive fingerprint via UART</li>
      <li><strong>Feedback:</strong> Vibration motor (GPIO-PWM)</li>
      <li><strong>Power:</strong> 3.7 V Li-Po (500 mAh) with onboard charger (~24 h runtime)</li>
    </ul>
    <h3>Room Gateway</h3>
    <ul>
      <li><strong>MCU:</strong> ESP32-WROOM-32 with high-gain antenna</li>
      <li><strong>Role:</strong> BLE central – subscribes to connection callbacks</li>
      <li><strong>Uplink:</strong> Wi-Fi/Ethernet → MQTT or HTTPS</li>
    </ul>
    <h3 >Cloud Integration</h3>
    <ul>
      <li><strong>Database:</strong> Firebase Realtime Database for event logging</li>
      <li><strong>Dashboard:</strong> Web UI (JavaScript) with real-time charts and audit reports</li>
    </ul>
    <h3>Fallback: Local Machine</h3>
    <ul>
      <li><strong>Server:</strong> Flask HTTP endpoint (<code>/log</code>)</li>
      <li><strong>Storage:</strong> Plain text log files + timestamped backups</li>
      <li><strong>Interface:</strong> Auto-refresh web dashboard (2 s) with local analytics</li>
    </ul>
  </section>

  <section id="comparative-table">
    <h2>Comparative Table</h2>
    <table>
      <thead>
        <tr>
          <th>#</th>
          <th>Aspect</th>
          <th>Common Across Patents</th>
          <th>Proposed Novel Approaches</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>1</td>
          <td>Check‑in/out Logging</td>
          <td>Central DB + manual scans</td>
          <td>Continuous BLE + manual fallback</td>
        </tr>
        <tr>
          <td>2</td>
          <td>Central DB Queries</td>
          <td>Cloud-only logs</td>
          <td>Edge anonymized logs + merged on positive case</td>
        </tr>
        <tr>
          <td>3</td>
          <td>Data Minimization</td>
          <td>UID + minimal personal data</td>
          <td>On-device biometrics + rotating BLE IDs</td>
        </tr>
        <tr>
          <td>4</td>
          <td>Scanning Tech</td>
          <td>BLE/RFID/QR</td>
          <td>100% touchless BLE auto + QR fallback</td>
        </tr>
        <tr>
          <td>5</td>
          <td>Admin Tools</td>
          <td>Web portal</td>
          <td>AI heatmaps & on-device risk scoring</td>
        </tr>
        <tr>
          <td>6</td>
          <td>ML Integration</td>
          <td>Server-side only</td>
          <td>On-device anomaly detection</td>
        </tr>
        <tr>
          <td>7</td>
          <td>Zonal Indicators</td>
          <td>Static LED colors</td>
          <td>Dynamic haptic + color/beep alerts</td>
        </tr>
        <tr>
          <td>8</td>
          <td>Health Status</td>
          <td>Minimal token</td>
          <td>Local test-status tokens + BLE alerts</td>
        </tr>
      </tbody>
    </table>
  </section>

  <section id="workflow">
    <h2>Workflow</h2>
    <ol>
      <li><strong>Automatic Association:</strong> Wearable enters Room A; gateway connects → <code>t_enter</code>.</li>
      <li><strong>Random Challenge:</strong> Gateway issues <code>PROVE-PRESENCE</code>; vibration until fingerprint match or timeout.</li>
      <li><strong>Strike Handling:</strong> Each failed scan increments counter; 3 fails → ALERT + buzz.</li>
      <li><strong>Seamless Handover:</strong> Exiting Room A → <code>t_exit</code>; entering Room B starts new session.</li>
      <li><strong>Cloud Sync:</strong> Batch POST every minute or on session end; backend aggregates for dashboards.</li>
    </ol>
  </section>

  <section id="privacy-security">
    <h2>Privacy & Security Considerations</h2>
    <ul>
      <li>Fingerprint templates stored and matched locally; never uploaded.</li>
      <li>All communications secured via Firebase authentication and TLS.</li>
    </ul>
  </section>

  <section id="financial-analysis">
    <h2>Financial Analysis (EGP)</h2>
    <table>
      <thead>
        <tr>
          <th>Component</th>
          <th>Qty</th>
          <th>Unit Cost</th>
          <th>Subtotal</th>
        </tr>
      </thead>
      <tbody>
        <tr>
          <td>Wearable (ESP32-C3)</td><td>1</td><td>200 EGP</td><td>200 EGP</td>
        </tr>
        <tr>
          <td>Fingerprint Sensor</td><td>1</td><td>450 EGP</td><td>450 EGP</td>
        </tr>
        <tr>
          <td>Vibration Motor</td><td>1</td><td>25 EGP</td><td>25 EGP</td>
        </tr>
        <tr>
          <td>Li-Po + Charger</td><td>1</td><td>60 EGP</td><td>60 EGP</td>
        </tr>
        <tr>
          <th>Wearable Total</th><td></td><td></td><th>735 EGP</th>
        </tr>
        <tr>
          <td>Room Gateway + PoE</td><td>1</td><td>400 EGP</td><td>400 EGP</td>
        </tr>
        <tr>
          <td>Cloud Hosting (Firebase Spark)</td><td>—</td><td>Free</td><td>0 EGP</td>
        </tr>
        <tr>
          <th>Per-Room Total</th><td></td><td></td><th>400 EGP</th>
        </tr>
      </tbody>
    </table>
  </section>

  <section id="differentiation">
    <h2>Differentiation vs. Existing Solutions</h2>
    <ul>
      <li><strong>BLE-Only Mesh:</strong> Simpler and cheaper than hybrid RFID/BLE.</li>
      <li><strong>Biometric Presence Proof:</strong> Prevents badge/device sharing.</li>
      <li><strong>Real-Time Alerts:</strong> Immediate admin notifications on tamper events.</li>
      <li><strong>Privacy-First:</strong> No audio/video, only room-level localization.</li>
    </ul>
  </section>


  <footer>
    &copy; 2025 The American University in Cairo. All rights reserved.
  </footer>

</body>
</html>
