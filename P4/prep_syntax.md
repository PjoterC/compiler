# Preparation Appointments - Syntax

[link spec](http://tph.tuwien.ac.at/~oemer/doc/quprog.pdf)

## QCL

### classical programming

```qcl
int Fibonacci(int n) { 	// calculate the n-th
	int i; 				// Fibonacci number
	int f; 				// by iteration
	for i = 1 to n {
		f = 2*f+i;
	}
	return f;
}
```


### quantum stuff
```sh
$ qcl -b10 # start qcl-interpreter with 10 qubits
```
```qcl
qcl> qureg a[4]; 	// allocate a 4-qubit register
qcl> qureg b[3]; 	// allocate another 3-qubit register
qcl> print a,b; 	// show actual qubit mappings
: |......3210> |...210....>
qcl> qureg c[5]; // try to allocate another 5 qubits
! memory error: not enough quantum memory
```