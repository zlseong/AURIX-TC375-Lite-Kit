# ==============================================================================
# Zonal Gateway Dependencies Download Script
# ==============================================================================
# This script downloads and extracts:
# 1. lwIP 2.1.3 (Lightweight IP stack)
# 2. FreeRTOS Kernel 10.5.1 (Real-Time Operating System)
# ==============================================================================

# Configuration
$WorkspaceRoot = $PSScriptRoot
$LwipDir = Join-Path $WorkspaceRoot "lwip"
$FreeRTOSDir = Join-Path $WorkspaceRoot "FreeRTOS"

# URLs
$LwipUrl = "https://download.savannah.nongnu.org/releases/lwip/lwip-2.1.3.zip"
$FreeRTOSUrl = "https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V10.5.1.zip"

# Temp directory
$TempDir = Join-Path $env:TEMP "zonal_gateway_deps"

# ==============================================================================
# Helper Functions
# ==============================================================================

function Write-ColoredMessage {
    param(
        [string]$Message,
        [string]$Color = "White"
    )
    Write-Host $Message -ForegroundColor $Color
}

function Download-File {
    param(
        [string]$Url,
        [string]$OutputPath
    )
    
    Write-ColoredMessage "📥 Downloading from: $Url" "Cyan"
    
    try {
        # Use System.Net.WebClient for better progress reporting
        $webClient = New-Object System.Net.WebClient
        $webClient.DownloadFile($Url, $OutputPath)
        Write-ColoredMessage "✅ Downloaded: $OutputPath" "Green"
        return $true
    }
    catch {
        Write-ColoredMessage "❌ Download failed: $_" "Red"
        return $false
    }
}

function Extract-ZipFile {
    param(
        [string]$ZipPath,
        [string]$DestinationPath
    )
    
    Write-ColoredMessage "📦 Extracting: $ZipPath" "Cyan"
    
    try {
        # Create destination directory if it doesn't exist
        if (!(Test-Path $DestinationPath)) {
            New-Item -ItemType Directory -Path $DestinationPath -Force | Out-Null
        }
        
        # Extract using .NET built-in
        Add-Type -AssemblyName System.IO.Compression.FileSystem
        [System.IO.Compression.ZipFile]::ExtractToDirectory($ZipPath, $DestinationPath)
        
        Write-ColoredMessage "✅ Extracted to: $DestinationPath" "Green"
        return $true
    }
    catch {
        Write-ColoredMessage "❌ Extraction failed: $_" "Red"
        return $false
    }
}

# ==============================================================================
# Main Script
# ==============================================================================

Write-ColoredMessage "`n========================================" "Yellow"
Write-ColoredMessage " Zonal Gateway Dependencies Downloader" "Yellow"
Write-ColoredMessage "========================================`n" "Yellow"

# Create temp directory
if (!(Test-Path $TempDir)) {
    New-Item -ItemType Directory -Path $TempDir -Force | Out-Null
}

# ==============================================================================
# 1. Download and Extract lwIP
# ==============================================================================

Write-ColoredMessage "`n[1/2] lwIP 2.1.3" "Yellow"
Write-ColoredMessage "─────────────────`n" "Yellow"

$lwipZip = Join-Path $TempDir "lwip-2.1.3.zip"
$lwipExtractPath = Join-Path $TempDir "lwip-extract"

if (Download-File -Url $LwipUrl -OutputPath $lwipZip) {
    if (Extract-ZipFile -ZipPath $lwipZip -DestinationPath $lwipExtractPath) {
        # Move to final location
        $lwipSourceDir = Join-Path $lwipExtractPath "lwip-2.1.3"
        $lwipFinalDir = Join-Path $LwipDir "lwip-2.1.3"
        
        if (Test-Path $lwipFinalDir) {
            Write-ColoredMessage "⚠️  lwIP directory already exists, removing..." "Yellow"
            Remove-Item -Path $lwipFinalDir -Recurse -Force
        }
        
        Move-Item -Path $lwipSourceDir -Destination $lwipFinalDir -Force
        Write-ColoredMessage "✅ lwIP installed to: $lwipFinalDir" "Green"
    }
}

# ==============================================================================
# 2. Download and Extract FreeRTOS
# ==============================================================================

Write-ColoredMessage "`n[2/2] FreeRTOS Kernel 10.5.1" "Yellow"
Write-ColoredMessage "─────────────────────────────`n" "Yellow"

$freertosZip = Join-Path $TempDir "FreeRTOS-Kernel-10.5.1.zip"
$freertosExtractPath = Join-Path $TempDir "freertos-extract"

if (Download-File -Url $FreeRTOSUrl -OutputPath $freertosZip) {
    if (Extract-ZipFile -ZipPath $freertosZip -DestinationPath $freertosExtractPath) {
        # Move to final location
        $freertosSourceDir = Join-Path $freertosExtractPath "FreeRTOS-Kernel-10.5.1"
        $freertosFinalDir = Join-Path $FreeRTOSDir "FreeRTOS-Kernel"
        
        if (Test-Path $freertosFinalDir) {
            Write-ColoredMessage "⚠️  FreeRTOS directory already exists, removing..." "Yellow"
            Remove-Item -Path $freertosFinalDir -Recurse -Force
        }
        
        # Create FreeRTOS directory if it doesn't exist
        if (!(Test-Path $FreeRTOSDir)) {
            New-Item -ItemType Directory -Path $FreeRTOSDir -Force | Out-Null
        }
        
        Move-Item -Path $freertosSourceDir -Destination $freertosFinalDir -Force
        Write-ColoredMessage "✅ FreeRTOS installed to: $freertosFinalDir" "Green"
    }
}

# ==============================================================================
# Cleanup
# ==============================================================================

Write-ColoredMessage "`n🧹 Cleaning up temporary files..." "Cyan"
if (Test-Path $TempDir) {
    Remove-Item -Path $TempDir -Recurse -Force
}
Write-ColoredMessage "✅ Cleanup complete" "Green"

# ==============================================================================
# Verify Installation
# ==============================================================================

Write-ColoredMessage "`n========================================" "Yellow"
Write-ColoredMessage " Installation Verification" "Yellow"
Write-ColoredMessage "========================================`n" "Yellow"

$allGood = $true

# Check lwIP
$lwipCoreDir = Join-Path $LwipDir "lwip-2.1.3\src\core"
if (Test-Path $lwipCoreDir) {
    Write-ColoredMessage "✅ lwIP: OK" "Green"
    Write-ColoredMessage "   Location: $LwipDir\lwip-2.1.3" "Gray"
} else {
    Write-ColoredMessage "❌ lwIP: NOT FOUND" "Red"
    $allGood = $false
}

# Check FreeRTOS
$freertosIncludeDir = Join-Path $FreeRTOSDir "FreeRTOS-Kernel\include"
if (Test-Path $freertosIncludeDir) {
    Write-ColoredMessage "✅ FreeRTOS: OK" "Green"
    Write-ColoredMessage "   Location: $FreeRTOSDir\FreeRTOS-Kernel" "Gray"
} else {
    Write-ColoredMessage "❌ FreeRTOS: NOT FOUND" "Red"
    $allGood = $false
}

# ==============================================================================
# Next Steps
# ==============================================================================

Write-ColoredMessage "`n========================================" "Yellow"
Write-ColoredMessage " Next Steps" "Yellow"
Write-ColoredMessage "========================================`n" "Yellow"

if ($allGood) {
    Write-ColoredMessage "✅ All dependencies downloaded successfully!`n" "Green"
    
    Write-ColoredMessage "📋 TODO:" "Cyan"
    Write-ColoredMessage "1. Create FreeRTOS configuration (FreeRTOSConfig.h)" "White"
    Write-ColoredMessage "2. Add include paths to Eclipse project:" "White"
    Write-ColoredMessage "   - lwip/lwip-2.1.3/src/include" "Gray"
    Write-ColoredMessage "   - lwip/port" "Gray"
    Write-ColoredMessage "   - FreeRTOS/FreeRTOS-Kernel/include" "Gray"
    Write-ColoredMessage "   - FreeRTOS/FreeRTOS-Kernel/portable/GCC/TriCore" "Gray"
    Write-ColoredMessage "3. Add source files to build configuration" "White"
    Write-ColoredMessage "4. Configure FreeRTOSConfig.h for TC375" "White"
    Write-ColoredMessage "5. Rebuild project`n" "White"
} else {
    Write-ColoredMessage "⚠️  Some dependencies failed to download." "Red"
    Write-ColoredMessage "Please check the error messages above and try again.`n" "Red"
}

Write-ColoredMessage "========================================`n" "Yellow"

# Pause to let user read the output
Write-Host "Press any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")

