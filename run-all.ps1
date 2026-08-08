#Requires -Version 5.1
<#
.SYNOPSIS
  Machine-checked artifacts for pa(5;6) = 31 and z(43;2) <= 294.

.DESCRIPTION
  .\run-all.ps1           everything, about 5 minutes
  .\run-all.ps1 -Quick    everything except the decisive N=32 run, about 40 s
  .\run-all.ps1 -Audit    also re-run the decisive case with the hole-pattern
                          sorting break switched off (adds roughly one hour)

  Requires only a C++17 compiler (MinGW g++, clang++ or MSVC cl) and Python 3.
  No third-party libraries, no downloads, no SAT solver. Every step asserts its
  expected outcome and the script halts at the first mismatch, so a clean finish
  is itself the proof.

  If Windows refuses to run this file because it came from a zip, use
    powershell -ExecutionPolicy Bypass -File .\run-all.ps1
  or run Unblock-File .\run-all.ps1 first.
#>
param(
    [switch]$Quick,
    [switch]$Audit
)

$ErrorActionPreference = 'Continue'
$ProgressPreference    = 'SilentlyContinue'
try { $PSNativeCommandUseErrorActionPreference = $false } catch { }
$env:PYTHONUNBUFFERED = '1'

$Root = $PSScriptRoot
Set-Location -LiteralPath $Root
$BinDir  = Join-Path $Root 'bin'
$WorkDir = Join-Path $Root 'work'
New-Item -ItemType Directory -Force -Path $BinDir  | Out-Null
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $Root 'certs') | Out-Null

$NSteps = 13
if ($Quick) { $NSteps = 12 }
if ($Audit) { $NSteps = $NSteps + 1 }
$Script:StepNo = 0
$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()

function Rule { Write-Host ('-' * 60) }
function Step([string]$Text) {
    $Script:StepNo++
    Write-Host ''
    Rule
    Write-Host ('[{0}/{1}] {2}' -f $Script:StepNo, $NSteps, $Text)
    Rule
}
function Note([string]$Text) { Write-Host ('  ' + $Text) }
function Fail([string]$Text) {
    Write-Host ''
    Write-Host ('#' * 60)
    Write-Host ('  FAILED at step {0}: {1}' -f $Script:StepNo, $Text)
    Write-Host '  Nothing below this point has been checked.'
    Write-Host ('#' * 60)
    exit 1
}

Write-Host ('=' * 60)
Write-Host '   pa(5;6) = 31  and  z(43;2) <= 294'
Write-Host '   machine-checked certificates and exhaustive searches'
Write-Host ('=' * 60)

# ---------------------------------------------------------------- 1. toolchain
Step 'toolchain and build'

$Cxx = $null
$CxxKind = $null
foreach ($c in @('g++', 'clang++')) {
    $cmd = Get-Command $c -ErrorAction SilentlyContinue
    if ($cmd) { $Cxx = $cmd.Source; $CxxKind = 'gcc'; break }
}
if (-not $Cxx) {
    $cmd = Get-Command 'cl' -ErrorAction SilentlyContinue
    if ($cmd) { $Cxx = $cmd.Source; $CxxKind = 'msvc' }
}
if (-not $Cxx) {
    Fail 'no C++ compiler on PATH. Install MSYS2/MinGW-w64 g++, or start a Visual Studio Developer PowerShell.'
}

$PyExe = $null
$PyPre = @()
foreach ($name in @('python', 'python3', 'py')) {
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if (-not $cmd) { continue }
    $pre = @()
    if ($name -eq 'py') { $pre = @('-3') }
    & $cmd.Source @pre '-c' 'import sys; sys.exit(0 if sys.version_info[0]==3 else 1)' 2>$null
    if ($LASTEXITCODE -eq 0) { $PyExe = $cmd.Source; $PyPre = $pre + @('-u'); break }
}
if (-not $PyExe) { Fail 'no Python 3 on PATH' }

Note ('compiler : ' + $Cxx)
Note ('python   : ' + $PyExe)

$SeedsExe  = Join-Path $BinDir 'seeds.exe'
$SearchExe = Join-Path $BinDir 'searchA.exe'
$OrbitsExe = Join-Path $BinDir 'orbits.exe'
$RefuteExe = Join-Path $BinDir 'refute.exe'
$CensusExe = Join-Path $BinDir 'census.exe'
$Sources   = @(
    @{ src = (Join-Path $Root 'src\seeds.cpp');   out = $SeedsExe  },
    @{ src = (Join-Path $Root 'src\searchA.cpp'); out = $SearchExe },
    @{ src = (Join-Path $Root 'src\orbits.cpp');  out = $OrbitsExe },
    @{ src = (Join-Path $Root 'src\refute.cpp');  out = $RefuteExe },
    @{ src = (Join-Path $Root 'src\census.cpp');  out = $CensusExe }
)
foreach ($t in $Sources) {
    if ($CxxKind -eq 'gcc') {
        & $Cxx -O2 -std=c++17 -o $($t.out) $($t.src)
    } else {
        $obj = Join-Path $BinDir ([System.IO.Path]::GetFileNameWithoutExtension($t.src) + '.obj')
        & $Cxx /nologo /O2 /EHsc /std:c++17 ('/Fe:' + $t.out) ('/Fo:' + $obj) $($t.src)
    }
    if ($LASTEXITCODE -ne 0) { Fail ('compiling ' + $t.src) }
}
Note 'built bin\seeds.exe bin\searchA.exe bin\orbits.exe bin\refute.exe bin\census.exe'

# ----------------------------------------------------------- 2,3,4. certificates
Step 'lower bound: pa(5;6) >= 31   (certificate, checked from first principles)'
Note '31 words over a 6-letter alphabet, length 5, pairwise distance >= 4.'
& $PyExe @PyPre (Join-Path $Root 'verify\verify.py') (Join-Path $Root 'certs\w31.txt') 6 4
if ($LASTEXITCODE -ne 0) { Fail 'certs\w31.txt is not a valid (5,31,4)_6 code' }

Step 'lower bound: A_6(6,5) >= 31   (certificate for the length-6 extension)'
Note 'the same 31 words extended to a sixth coordinate; deleting it recovers'
Note 'certs\w31.txt exactly, so this certificate implies the previous one.'
& $PyExe @PyPre (Join-Path $Root 'verify\verify.py') (Join-Path $Root 'certs\w31_6.txt') 6 5
if ($LASTEXITCODE -ne 0) { Fail 'certs\w31_6.txt is not a valid (6,31,5)_6 code' }

Step 'a 43x43 configuration with 290 incidences exists   (certificate)'
Note 'no two points on two common lines, checked over all 903 point pairs'
Note 'and, independently, over all 903 line pairs. Nothing is read from the'
Note 'file header: every quantity is recomputed by the verifier.'
& $PyExe @PyPre (Join-Path $Root 'verify\verify_cert290.py') (Join-Path $Root 'certs\cert_290.txt') 290
if ($LASTEXITCODE -ne 0) { Fail 'certs\cert_290.txt is not a valid 290-incidence configuration' }

# ------------------------------------------------------------------- 5. seeds
Step 'canonical seed lists (orbit representatives for grid row 1)'
$Seeds2 = Join-Path $WorkDir 'seeds2.txt'
$Seeds3 = Join-Path $WorkDir 'seeds3.txt'

$out2 = & $SeedsExe 2
if ($LASTEXITCODE -ne 0) { Fail 'seeds 2 failed its built-in count check' }
$out2 | Set-Content -LiteralPath $Seeds2 -Encoding ascii

$out3 = & $SeedsExe 3
if ($LASTEXITCODE -ne 0) { Fail 'seeds 3 failed its built-in count check' }
$out3 | Set-Content -LiteralPath $Seeds3 -Encoding ascii
Note 'wrote work\seeds2.txt and work\seeds3.txt'

# ------------------------------------------------------- 6. seed completeness
Step 'independent completeness check of the seed lists'
Note 'This is the one step that could silently void the upper bound,'
Note 'so it is re-derived here by a second implementation, in Python.'
Write-Host ''
& $PyExe @PyPre (Join-Path $Root 'verify\checkseeds.py') $Seeds2
if ($LASTEXITCODE -ne 0) { Fail 'S=2 seed list is not complete' }
Write-Host ''
& $PyExe @PyPre (Join-Path $Root 'verify\checkseeds.py') $Seeds3
if ($LASTEXITCODE -ne 0) { Fail 'S=3 seed list is not complete' }

# ---------------------------------------------------------------- 7,8,9. controls
Step 'control against the literature: a 34-word code at K=4 exists'
Note 'A_6(4,3) = 34 is known independently; the search must find one.'
& $SearchExe 2 34 $Seeds2 -expect F -every 5
if ($LASTEXITCODE -ne 0) { Fail 'K=4 N=34 did not come out FEASIBLE' }

Step 'control against the literature: no 35-word code at K=4 exists'
Note 'the same known value from the other side; the search must refute it.'
& $SearchExe 2 35 $Seeds2 -expect I -every 5
if ($LASTEXITCODE -ne 0) { Fail 'K=4 N=35 did not come out INFEASIBLE' }

Step 'positive control at K=5: a 30-word code exists'
Note 'guards against a search that refutes everything.'
& $SearchExe 3 30 $Seeds3 -expect F -every 5
if ($LASTEXITCODE -ne 0) { Fail 'K=5 N=30 did not come out FEASIBLE' }

# ------------------------------------------- 10,11. independent confirmation
Step 'independent reduction: 56 orbits under S6 x S4'
Note 'a second, separately written program that shares no code with searchA.'
$OrbRef = Join-Path $WorkDir 'orbits56.ref'
$OrbOut = Join-Path $Root 'data\orbits56.txt'
if (-not (Test-Path -LiteralPath $OrbOut)) { Fail 'data\orbits56.txt is missing' }
Copy-Item -LiteralPath $OrbOut -Destination $OrbRef -Force
& $OrbitsExe
if ($LASTEXITCODE -ne 0) { Fail 'orbits failed' }
$a = Get-FileHash -LiteralPath $OrbRef -Algorithm SHA256
$b = Get-FileHash -LiteralPath $OrbOut -Algorithm SHA256
if ($a.Hash -ne $b.Hash) { Fail 'the regenerated orbit list differs from the shipped data\orbits56.txt' }
Note 'regenerated data\orbits56.txt is byte-identical to the shipped reference'

Step 'independent confirmation: exact maximum over the 56 orbits is 18'
Note 'a total of 32 words needs 20 further words on top of the 12 already'
Note 'placed, so any maximum below 20 refutes it; 18 clears it by two.'
Write-Host ''
& $RefuteExe
if ($LASTEXITCODE -ne 0) { Fail 'refute failed' }

# ------------------------------------------------------------------- 12. census
Step 'parameter census: the eight admissible (n8,m8) pairs at E = 295'
Note 'bounded-knapsack enumeration for Proposition 8.3, swept over the full'
Note 'a priori range 0 <= n8,m8 <= 24 given by Cauchy-Schwarz. Deterministic,'
Note 'single-threaded, no input files, about 15 seconds.'
Write-Host ''
& $CensusExe -quiet
if ($LASTEXITCODE -ne 0) { Fail 'the census output does not match the list in Proposition 8.3' }

# ----------------------------------------------------------------- 13. decisive
$SumPath = Join-Path $WorkDir 'n32.sum'
if (-not $Quick) {
    Step 'DECISIVE: no 32-word code at K=5  (this is the upper bound)'
    Note '103 seeds x 696 hole patterns, exhaustive. Expect about 4 minutes.'
    Write-Host ''
    & $SearchExe 3 32 $Seeds3 -expect I -sum $SumPath -every 2
    if ($LASTEXITCODE -ne 0) { Fail 'K=5 N=32 did not come out INFEASIBLE' }
}

# -------------------------------------------------------------------- 14. audit
if ($Audit) {
    Step 'audit: same case with the hole-pattern sorting break disabled'
    Note 'enumerates all C(24,4) = 10626 patterns instead of the 696 sorted ones.'
    Note 'This takes roughly an hour and is not needed for the proof; the break'
    Note 'is justified by a counting argument in PROOF.md section 6.'
    Write-Host ''
    & $SearchExe 3 32 $Seeds3 -allpat -expect I -sum (Join-Path $WorkDir 'n32_allpat.sum') -every 10
    if ($LASTEXITCODE -ne 0) { Fail 'the -allpat audit did not come out INFEASIBLE' }
}

# -------------------------------------------------------------------- summary
$Stopwatch.Stop()
$Elapsed = [int]$Stopwatch.Elapsed.TotalSeconds

$sum = @{}
if (Test-Path -LiteralPath $SumPath) {
    foreach ($line in Get-Content -LiteralPath $SumPath) {
        if ($line -match '^([A-Za-z_]+)=(.*)$') { $sum[$Matches[1]] = $Matches[2] }
    }
}

Write-Host ''
Write-Host ''
Write-Host ('=' * 60)
if ($Quick) {
    Write-Host '   QUICK MODE -- the decisive run was skipped'
    Write-Host ('=' * 60)
    Write-Host '   Every certificate and every control passed.'
    Write-Host '   Run .\run-all.ps1 with no flags for the N=32 refutation.'
    Write-Host ('   elapsed: {0}s' -f $Elapsed)
    Write-Host ('=' * 60)
    exit 0
}
Write-Host '   ALL STAGES PASSED       pa(5;6) = A_6(5,4) = 31'
Write-Host ('=' * 60)
Write-Host '   lower bound   >= 31   certs\w31.txt verified from scratch:'
Write-Host '                         465 pairs, minimum Hamming distance 4'
Write-Host '   length-6      >= 31   certs\w31_6.txt verified, distance 5'
Write-Host '   configuration         certs\cert_290.txt verified: 43 points,'
Write-Host '                         290 incidences, profiles 7^32 6^11, leave 66'
Write-Host '   upper bound   <= 31   no 32-word code exists:'
Write-Host ('                         {0} seeds x {1} hole patterns = {2} subproblems' -f $sum['sum_seeds'], $sum['sum_patterns'], $sum['sum_subproblems'])
Write-Host ('                         {0} nodes, {1}s, {2}' -f $sum['sum_nodes'], $sum['sum_time'], $sum['sum_status'])
Write-Host '   reduction             103 orbit representatives cover all 393120'
Write-Host '                         4x6 Latin rectangles: 0 missing, 0 spurious'
Write-Host '   independent route     56-orbit reduction, exact maximum 18,'
Write-Host '                         against the 20 further words a 32-word code'
Write-Host '                         would need: refuted by a margin of two'
Write-Host '   parameter census      Proposition 8.3 reproduced: exactly the eight'
Write-Host '                         pairs (0,0) (0,1) (1,0) (1,1) (1,2) (2,1)'
Write-Host '                         (2,2) (3,3) over 0 <= n8,m8 <= 24'
Write-Host '   controls              A_6(4,3) = 34 reproduced from both sides;'
Write-Host '                         30-word code at K=5 found (positive control)'
Write-Host ('   total wall clock      {0}s' -f $Elapsed)
Write-Host ('=' * 60)
Write-Host '   Reference run: 1427441869 nodes. The search is deterministic,'
Write-Host '   so the node count above should match exactly on any machine.'
Write-Host ('=' * 60)
exit 0
