$outputDir = ".\test_results"
if (-not (Test-Path $outputDir)) {
    New-Item -Path $outputDir -ItemType Directory
}

for ($i = 1; $i -le 21; $i++) {
    Start-Sleep -Seconds 1
    $inputFile = ".\tests\test$i.txt"
    $outputFile = ".\test_results\test${i}results.txt"

    if (Test-Path $inputFile) {
        Get-Content $inputFile | .\tma3.exe > $outputFile
        Write-Host "Processed $inputFile and saved output to $outputFile"
    } else {
        Write-Host "Warning: $inputFile not found. Skipping."
    }
}
Write-Host "All tests completed."