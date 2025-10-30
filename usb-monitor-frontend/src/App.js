import React, { useState, useEffect } from 'react';

function App() {
  const [devices, setDevices] = useState([]);
  const [loggedIn, setLoggedIn] = useState(false);
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [loginError, setLoginError] = useState("");

  useEffect(() => {
    if (!loggedIn) return;
    const fetchDevices = () => {
      fetch("/api/devices")
        .then(res => res.json())
        .then(data => {
          console.log("Fetched devices:", data);
          setDevices(data);
        })
        .catch((e) => {
          console.error("Error fetching devices:", e);
          setDevices([]);
        });
    };
    fetchDevices();
    const interval = setInterval(fetchDevices, 30000);
    return () => clearInterval(interval);
  }, [loggedIn]);

  const allowDevice = deviceId => {
    fetch("/api/device/allow", { method: "POST", headers: { "Content-Type": "text/plain" }, body: deviceId })
      .then(() => {/* auto refresh by poll */});
  };
  const denyDevice = deviceId => {
    fetch("/api/device/deny", { method: "POST", headers: { "Content-Type": "text/plain" }, body: deviceId })
      .then(() => {/* auto refresh by poll */});
  };

  const handleLogin = () => {
    if (username === "admin" && password === "secure123") {
      setLoggedIn(true); setLoginError("");
    } else {
      setLoginError("Invalid credentials");
    }
  };

  if (!loggedIn) {
    return (
      <div style={{background:'#232439',height:'100vh',display:'flex',alignItems:'center',justifyContent:'center'}}>
        <div style={{background:'#181b2a',padding:32,borderRadius:12}}>
          <h2 style={{color:'#fff'}}>Admin Login</h2>
          <input type="text" placeholder="Username" value={username} onChange={e=>setUsername(e.target.value)}
            style={{marginBottom:8,padding:8,width:'90%'}} />
          <br />
          <input type="password" placeholder="Password" value={password} onChange={e=>setPassword(e.target.value)}
            style={{marginBottom:8,padding:8,width:'90%'}} />
          <br />
          <button onClick={handleLogin} style={{padding:'8px 24px'}}>Login</button>
          {loginError && <div style={{color:'red',marginTop:8}}>{loginError}</div>}
        </div>
      </div>
    );
  }

  return (
    <div style={{padding: '32px', background: '#232439', minHeight: '100vh', color: '#fff'}}>
      <h1 style={{marginBottom:24}}>USB Device Monitoring Dashboard</h1>
      <table style={{width: '100%', background: '#202235', marginTop: 20, borderCollapse: 'collapse'}}>
        <thead>
          <tr>
            <th style={{padding:12}}>Device ID</th>
            <th style={{padding:12}}>Name</th>
            <th style={{padding:12}}>Status</th>
            <th style={{padding:12}}>Path</th>
            <th style={{padding:12}}>Actions</th>
          </tr>
        </thead>
        <tbody>
          {
            devices.length === 0
              ? (
                <tr>
                  <td colSpan={5} style={{textAlign: 'center', color: '#999', fontStyle: 'italic'}}>
                    No devices recognized. <br/>
                    <span style={{fontSize: 'smaller'}}>DEBUG: {JSON.stringify(devices)}</span>
                  </td>
                </tr>
              )
              : devices.map(dev => (
                <tr key={dev.deviceId}>
                  <td style={{padding:10}}>{dev.deviceId}</td>
                  <td style={{padding:10}}>{dev.friendlyName}</td>
                  <td style={{padding:10}}>
                    {dev.isAllowed ?
                      <span style={{color:'limegreen', fontWeight:'bold'}}>🟢 Allowed</span> :
                      <span style={{color:'crimson', fontWeight:'bold'}}>🔴 Blocked</span>
                    }
                  </td>
                  <td style={{padding:10}}>{dev.devicePath}</td>
                  <td style={{padding:10}}>
                    <button disabled={dev.isAllowed} onClick={()=>allowDevice(dev.deviceId)}>
                      Allow
                    </button>
                    <button disabled={!dev.isAllowed} onClick={()=>denyDevice(dev.deviceId)}>
                      Deny
                    </button>
                  </td>
                </tr>
            ))
          }
        </tbody>
      </table>
    </div>
  );
}

export default App;
