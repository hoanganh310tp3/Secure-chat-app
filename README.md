# Check if port 2000 is in use
netstat -tulpn | grep 2000

# Check all listening ports
netstat -tulpn | grep LISTEN

# Check specific port with more details
netstat -tulpn | grep :2000

# Check what's using port 2000
lsof -i :2000

# Check all network connections
lsof -i

# Check TCP connections on port 2000
lsof -i tcp:2000

# Find the process ID (PID) using the port
lsof -ti:2000

# Kill the process (replace PID with actual number)
kill -9 PID

# Or kill directly
fuser -k 2000/tcp
