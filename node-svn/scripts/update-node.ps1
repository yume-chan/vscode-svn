$version = node -v
$version = $version.substring(1)

$sdk_dir = "lib/node-sdk"

Write-Output "Downloading SDK for Node $version..."

node-gyp.cmd install $version --devdir $sdk_dir --disturl=https://npm.taobao.org/mirrors/node *> $null

Write-Output "Extracting..."

Remove-Item -Path include/node -Recurse *> $null
Copy-Item -Path $sdk_dir/$version/include/node -Destination include/node -Recurse *> $null

Remove-Item -Path lib/node/* -Recurse *> $null

$platform = "win32"

New-Item -Path "lib/node/$platform/x64" -ItemType Directory *> $null
Copy-Item -Path "$sdk_dir/$version/x64/*" -Destination "lib/node/$platform/x64" -Recurse *> $null

New-Item -Path "lib/node/$platform/x86" -ItemType Directory *> $null
Copy-Item -Path "$sdk_dir/$version/ia32/*" -Destination "lib/node/$platform/x86" -Recurse *> $null

Remove-Item -Path $sdk_dir -Recurse *> $null

Write-Output "Done"
