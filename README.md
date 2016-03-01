# ARPy
A minimal ARP spoofing tool that doesn't depend on any networking libraries.

## Installation
Clone the repository:
```
$: git clone https://github.com/dylanmm/ARPy.git
```
Go into the project directory:
```
$: cd ARPy/
```
Build with `make` or install with `make install`
```
$: make
$: sudo make install
```
`make` with build arpy and can then be run in the current directory. `make install` will put the binary into `/usr/local/bin`.

Uninstall with `make uninstall`
```
$: sudo make uninstall
```
Or uninstall by removing the binary file in `/usr/local/bin` with `rm /usr/local/bin/arpy`.

## Usage
```
arpy -d <interface device> -t <target IP> -T <target MAC> -r <router IP> -R <router MAC>
```
All flags are required.

## Example 



[LICENSE](LICENSE)
