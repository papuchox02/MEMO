$ErrorActionPreference = 'Stop'
$memoNode = (Get-Command node -ErrorAction SilentlyContinue).Source
if (-not $memoNode) {
  $memoNode = Join-Path $env:USERPROFILE '.cache/codex-runtimes/codex-primary-runtime/dependencies/node/bin/node.exe'
}
if (-not (Test-Path -LiteralPath $memoNode)) { throw 'Instala Node.js 20 o posterior.' }

Write-Host 'Direcciones IPv4 del PC. Usa la de tu red Wi-Fi en Configuracion.h:'
Get-NetIPConfiguration | Where-Object { $_.IPv4DefaultGateway } |
  ForEach-Object { Write-Host "$($_.InterfaceAlias): $($_.IPv4Address.IPAddress)" }

try {
  $memoSecret = Read-Host 'Clave API de Gemini' -AsSecureString
  $env:GEMINI_API_KEY = [System.Net.NetworkCredential]::new('', $memoSecret).Password
  $env:MEMO_TOKEN = Read-Host 'Codigo privado de MEMO (12 caracteres o mas, igual que en Configuracion.h)'
  & $memoNode (Join-Path $PSScriptRoot 'servidor.cjs')
  if ($LASTEXITCODE -ne 0) { throw 'El servidor no pudo continuar.' }
} finally {
  Remove-Item Env:GEMINI_API_KEY, Env:MEMO_TOKEN -ErrorAction SilentlyContinue
}
