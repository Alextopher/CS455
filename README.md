# CS455

## Instuctions 

Step by step here is how this project should be ran.

1. install NS3 (our program is tested using version 3.35)
2. Clone the repo within `ns-allinone-3.35/ns-3.35/src/`
3. **rename the cloned directory** to `dvhop`
4. cd back to `ns-allinone-3.35/ns-3.35/`
5. run `./waf --run src/dvhop/examples/dvhop-example` (and wait)
6. view the results.

If you want to change the ratio of beacons to nodes you should edit the example constuctor

```
DVHopExample::DVHopExample () :
  size (20),
  beacons(10),
  totalTime (10),
  pcap (false),
  printRoutes (false)
{
}
```

Note: Size is the total number of "beacons and nodes". For example in with the current code you will see 10 node and 10 beacons