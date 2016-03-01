# ARPy
A minimal ARP spoofing tool. Doesn't depend on any networking libraries.

## Installation
Clone the repository:
```
$: git clone https://github.com/dylanmm/ARPy.git
```
Go into the project directory:
```
$: cd ARPy/
```
Build with `make` and install with `make install`
```
$: make
$: sudo make install
```

Uninstall with `make uninstall`
```
$: sudo make uninstall
```

## Usage
```
arpy -d <interface device> -t <target IP> -T <target MAC> -r <router IP> -R <router MAC>
```


[LICENSE](LICENSE)
