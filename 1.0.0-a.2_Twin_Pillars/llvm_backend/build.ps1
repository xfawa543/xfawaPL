# Build script for the LLVM backend prototype (Windows PowerShell)
# Tries clang++ then g++ (assumes either is in PATH)

$src = "xfawac_llvm.cpp"
$out = "xfawac_llvm.exe"
Write-Host "Compiling $src -> $out"
if (Get-Command clang++ -ErrorAction SilentlyContinue) {
    clang++ -std=c++17 -O2 $src -o $out
} else {
    if (Get-Command g++ -ErrorAction SilentlyContinue) {
        g++ -std=c++17 -O2 $src -o $out
    } else {
        Write-Error "No clang++ or g++ found in PATH"
        exit 1
    }
}
Write-Host "Done. Run .\xfawac_llvm.exe <input.xf> [-o out.exe]"
