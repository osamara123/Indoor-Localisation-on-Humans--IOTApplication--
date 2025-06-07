#!/usr/bin/env python3
"""
BLE Presence Monitor – room‑aware
* Receives POSTs from multiple ESP32 scanners
  timestamp=<YYYY‑MM‑DD HH:MM:SS>
  room=<Room 1/Room 2/...>
  event=DETECTED|VIBRATE_SENT|LOST
  beacon=<UUID-major-minor>
* Stores plain‑text log               beacon_events.txt
* Shows pages:
    /           – event log
    /beacons    – current status for each beacon
    /duration/<beacon_id> – visit history + stats
    /clear      - delete all logs
  (auto‑refresh every 2 s)
"""

from flask import Flask, request, render_template_string, redirect, url_for
import datetime, os, shutil

app = Flask(_name_)
LOG_FILE = "beacon_events.txt"
BACKUP_DIR = "log_backups"

# ────────── HTML templates (compact, inline) ──────────
NAV_BAR = '''<div class="nav">
<a href="/">Event Log</a>
<a href="/beacons">Beacons</a>
<a href="/clear" style="color:#ff6b6b;">Clear Logs</a>
</div>'''

MAIN_HTML = '''
<!DOCTYPE html><html><head><title>BLE Presence</title>
<meta http-equiv="refresh" content="2">
<style>
 body{font-family:Arial;margin:0;padding:20px}
 h1{color:#2c3e50}table{width:100%;border-collapse:collapse;margin-top:20px}
 th{background:#2c3e50;color:#fff}th,td{padding:10px;border:1px solid #ddd;text-align:left}
 tr:nth-child(even){background:#f9f9f9}.nav{background:#2c3e50;margin:-20px -20px 20px;padding:10px}
 .nav a{color:#fff;margin-right:20px;text-decoration:none}
 .btn{padding:5px 10px;background:#3498db;color:white;border:none;cursor:pointer;text-decoration:none;display:inline-block}
 .btn.danger{background:#e74c3c;}
</style></head><body>
''' + NAV_BAR + '''
<h1>BLE Presence Events</h1>
<p>Refreshes every 2 s · Server time {{srv}} · <span>{{count}} events</span></p>

<table><tr><th>Timestamp</th><th>Room</th><th>Event</th><th>Beacon ID</th></tr>
{% for r in rows %}<tr><td>{{r.ts}}</td><td>{{r.room}}</td><td>{{r.ev}}</td><td>{{r.bid}}</td></tr>{% endfor %}
</table></body></html>'''

BEACON_HTML = '''
<!DOCTYPE html><html><head><title>BLE Beacons</title>
<meta http-equiv="refresh" content="2"><style>''' + MAIN_HTML.split("<style>")[1]

BEACON_HTML += '''
</style></head><body>
''' + NAV_BAR + '''
<h1>Beacon Status</h1><p>Refreshes every 2 s · Server time {{srv}}</p>
<table><tr><th>Beacon ID</th><th>Room</th><th>Status</th><th>Last seen</th><th>Total visits</th><th>Details</th></tr>
{% for b in beacons %}<tr><td>{{b.id}}</td><td>{{b.room}}</td><td>{{b.status}}</td>
<td>{{b.last}}</td><td>{{b.visits}}</td><td><a href="/duration/{{b.id}}">View</a></td></tr>{% endfor %}
</table></body></html>'''

DURATION_HTML = '''
<!DOCTYPE html><html><head><title>{{bid}} details</title>
<meta http-equiv="refresh" content="2">
<style>''' + MAIN_HTML.split("<style>")[1]

DURATION_HTML += '''
</style></head><body>
''' + NAV_BAR + '''
<h1>Activity – {{bid}}</h1>
<div style="background:#eaf2f8;padding:10px;border-radius:5px;margin-bottom:20px">
Total visits: {{tot}} · Total time: {{totd}} · Average visit: {{avg}}</div>
<table><tr><th>#</th><th>Room</th><th>Entry</th><th>Exit</th><th>Duration</th></tr>
{% for v in sess %}<tr><td>{{loop.index}}</td><td>{{v.room}}</td><td>{{v.start}}</td>
<td>{{v.end}}</td><td>{{v.dur}}</td></tr>{% endfor %}
</table></body></html>'''

CLEAR_HTML = '''
<!DOCTYPE html><html><head><title>Clear Logs</title>
<style>''' + MAIN_HTML.split("<style>")[1] + '''
.card{background:#f8f9fa;border-radius:5px;padding:20px;margin-bottom:20px}
.danger{background:#fee;border-left:4px solid #e74c3c;padding:10px}
</style></head><body>
''' + NAV_BAR + '''
<h1>Log Management</h1>

<div class="card">
    <h2>Clear All Logs</h2>
    <p class="danger">Warning: This will permanently delete all event logs.</p>
    <form method="post" action="/clear">
        <input type="hidden" name="action" value="clear_all">
        <button type="submit" class="btn danger">Delete All Logs</button>
    </form>
</div>

<div class="card">
    <h2>Backup Logs</h2>
    <p>Create a timestamped backup of the current logs</p>
    <form method="post" action="/clear">
        <input type="hidden" name="action" value="backup">
        <button type="submit" class="btn">Create Backup</button>
    </form>
</div>

<div class="card">
    <h2>Available Backups</h2>
    <table>
        <tr><th>Backup File</th><th>Size</th><th>Action</th></tr>
        {% for backup in backups %}
        <tr>
            <td>{{backup.name}}</td>
            <td>{{backup.size}} bytes</td>
            <td>
                <form method="post" action="/clear" style="display:inline">
                    <input type="hidden" name="action" value="restore">
                    <input type="hidden" name="file" value="{{backup.name}}">
                    <button type="submit" class="btn">Restore</button>
                </form>
            </td>
        </tr>
        {% endfor %}
    </table>
</div>
</body></html>'''

# ────────── helpers ──────────
def log_line(ts, room, ev, bid):
    with open(LOG_FILE, "a") as f:
        f.write(f"{ts} - {room} - {ev} - {bid}\n")

def parse(line):
    p = line.strip().split(" - ")
    return p if len(p) == 4 else None

def create_backup():
    """Create a timestamped backup of the log file"""
    if not os.path.exists(BACKUP_DIR):
        os.makedirs(BACKUP_DIR)
    
    if os.path.exists(LOG_FILE):
        timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_file = os.path.join(BACKUP_DIR, f"logs_{timestamp}.txt")
        shutil.copy2(LOG_FILE, backup_file)
        return backup_file
    return None

def get_backups():
    """Get list of available backup files"""
    backups = []
    if os.path.exists(BACKUP_DIR):
        for file in os.listdir(BACKUP_DIR):
            if file.startswith("logs_") and file.endswith(".txt"):
                path = os.path.join(BACKUP_DIR, file)
                size = os.path.getsize(path)
                backups.append({"name": file, "size": size})
    return backups

def restore_backup(backup_file):
    """Restore logs from backup file"""
    backup_path = os.path.join(BACKUP_DIR, backup_file)
    if os.path.exists(backup_path):
        shutil.copy2(backup_path, LOG_FILE)
        return True
    return False

# ────────── routes ──────────
@app.route("/log", methods=["POST"])
def log_event():
    ts   = request.form.get("timestamp",  "")
    room = request.form.get("room",      "Unknown")
    ev   = request.form.get("event",     "")
    bid  = request.form.get("beacon",    "")
    log_line(ts, room, ev, bid)
    print(ts, room, ev, bid, sep=" | ")
    return "OK"

@app.route("/")
def main():
    rows = []
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE) as f:
            for ln in f:
                p = parse(ln)
                if p:
                    rows.append({"ts":p[0],"room":p[1],"ev":p[2],"bid":p[3]})
    rows.reverse()
    return render_template_string(MAIN_HTML, 
                                  rows=rows,
                                  count=len(rows),
                                  srv=datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

@app.route("/beacons")
def beacons():
    d = {}  # {id:{room,status,last,visits}}
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE) as f:
            for ln in f:
                p = parse(ln)
                if not p: continue
                ts, room, ev, bid = p
                if bid not in d:
                    d[bid] = {"id":bid,"room":room,"status":"Unknown","last":ts,"visits":0}
                d[bid]["room"] = room
                d[bid]["last"] = ts
                if ev == "DETECTED":
                    d[bid]["status"] = "Present"; d[bid]["visits"] += 1
                elif ev == "LOST":
                    d[bid]["status"] = "Away"
    return render_template_string(BEACON_HTML, 
                                  beacons=list(d.values()),
                                  srv=datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"))

@app.route("/duration/<bid>")
def duration(bid):
    sessions = []
    cur = None
    if os.path.exists(LOG_FILE):
        with open(LOG_FILE) as f:
            for p in (parse(l) for l in f):
                if not p: continue
                ts, room, ev, b = p
                if b != bid: continue
                if ev=="DETECTED":
                    cur = {"start":ts,"room":room}
                elif ev=="LOST" and cur:
                    cur["end"]=ts
                    try:
                        fmt="%Y-%m-%d %H:%M:%S"
                        dur = datetime.datetime.strptime(cur["end"],fmt)-datetime.datetime.strptime(cur["start"],fmt)
                        cur["dur"]=str(dur)
                    except:
                        cur["dur"]="?"
                    sessions.append(cur); cur=None
    tot_vis=len(sessions)
    total_sec=sum((datetime.timedelta(hours=int(s["dur"].split(':')[0]),
                                      minutes=int(s["dur"].split(':')[1]),
                                      seconds=int(float(s["dur"].split(':')[2])))
                   .total_seconds() if "dur" in s and ":" in s["dur"] else 0)
                  for s in sessions)
    totd = str(datetime.timedelta(seconds=total_sec))
    avg  = str(datetime.timedelta(seconds=int(total_sec/tot_vis))) if tot_vis else "N/A"
    return render_template_string(DURATION_HTML,
                                  bid=bid,
                                  sess=sessions,
                                  tot=tot_vis,
                                  totd=totd,
                                  avg=avg)

@app.route("/clear", methods=["GET", "POST"])
def clear_logs():
    if request.method == "POST":
        action = request.form.get("action", "")
        
        if action == "clear_all":
            # Delete the log file
            if os.path.exists(LOG_FILE):
                # Create backup before deleting
                create_backup()
                # Clear the file
                open(LOG_FILE, 'w').close()
                return redirect(url_for('main'))
        
        elif action == "backup":
            # Create backup
            backup_file = create_backup()
            if backup_file:
                return "Backup created: " + backup_file + ' <a href="/clear">Back</a>'
            else:
                return "No log file to backup. <a href='/clear'>Back</a>"
        
        elif action == "restore":
            # Restore from backup
            backup_file = request.form.get("file", "")
            if restore_backup(backup_file):
                return "Logs restored from " + backup_file + ' <a href="/">View Logs</a>'
            else:
                return "Failed to restore backup. <a href='/clear'>Back</a>"
    
    # Show management page
    backups = get_backups()
    return render_template_string(CLEAR_HTML, backups=backups)

if _name_ == "_main_":
    print("Starting BLE Presence Monitor Server")
    print("Server ready: http://localhost:5000")
    
    # Ensure log file exists
    if not os.path.exists(LOG_FILE):
        open(LOG_FILE, 'a').close()
        
    # Create backup directory if needed
    if not os.path.exists(BACKUP_DIR):
        os.makedirs(BACKUP_DIR)
        
    app.run(host="0.0.0.0", port=5000, debug=False)