# Applies the classroom broker config. MUST be run in an elevated PowerShell
# (right-click PowerShell -> "Run as administrator"), because it writes into
# C:\Program Files, restarts a service, and adds firewall rules.
#
#   powershell -ExecutionPolicy Bypass -File .\apply-broker-config.ps1

$ErrorActionPreference = 'Stop'

$id = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($id)
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltinRole]::Administrator)) {
    Write-Error "This script needs an elevated PowerShell (Run as administrator)."
}

$target = "C:\Program Files\mosquitto\mosquitto.conf"
$source = Join-Path $PSScriptRoot "mosquitto.conf"

# Persistence file and log both live here.
$dataDir = "C:\ProgramData\mosquitto"
if (-not (Test-Path $dataDir)) {
    New-Item -ItemType Directory -Path $dataDir | Out-Null
    Write-Host "Created $dataDir"
}

# Back up whatever is there now, once.
$backup = "$target.bak"
if (-not (Test-Path $backup)) {
    Copy-Item $target $backup
    Write-Host "Backed up original config to $backup"
}

Copy-Item $source $target -Force
Write-Host "Wrote $target"

# Open both ports to the local network only (private profile).
foreach ($port in 1883, 9001) {
    $name = "Mosquitto MQTT $port"
    if (-not (Get-NetFirewallRule -DisplayName $name -ErrorAction SilentlyContinue)) {
        New-NetFirewallRule -DisplayName $name -Direction Inbound -Action Allow `
            -Protocol TCP -LocalPort $port -Profile Private | Out-Null
        Write-Host "Added firewall rule: $name (private profile)"
    } else {
        Write-Host "Firewall rule already present: $name"
    }
}

Restart-Service mosquitto
Start-Sleep -Seconds 2
$svc = Get-Service mosquitto
Get-Service mosquitto | Format-List Name, Status

# A bad config makes the service exit immediately and Restart-Service does not
# report why. Run the broker in the foreground to surface the parse error.
if ($svc.Status -ne 'Running') {
    Write-Warning "mosquitto did not stay running - config error below:"
    & "C:\Program Files\mosquitto\mosquitto.exe" -c $target -v
    exit 1
}

Write-Host "`nListening sockets:"
netstat -ano | Select-String ":1883|:9001"

Write-Host "`nBroker address for students:"
Get-NetIPAddress -AddressFamily IPv4 |
    Where-Object { $_.IPAddress -notlike "127.*" -and $_.IPAddress -notlike "169.254.*" } |
    Select-Object IPAddress, InterfaceAlias | Format-Table -AutoSize
