# Integration of SDN with Wireless Interfaces Including Cellular mmWave for Simulations in ns-3 v3.29

This code integrates modules from two repositories along with other improvements to expand the functionality to wireless interfaces.
This work aims to serve as a framework for simulation of scenarios integrating SDN on vehicular adhoc networks (VANET).

## Clonage Instructions

This repository uses library *ofsoftswitch13* as a submodule. Therefore, to clone the repository, use:

```
git clone --recurse-submodules https://github.com/ogbautista/ns3-mmwave-sdn-vanet.git
```

### Compiling *ofsoftswitch13* as a static library

To compile the *ofsoftswitch13* library, follow this steps:
```
cd src/ofswitch13/lib/ofsoftswitch13
./boot.sh
./configure --enable-ns3-lib
make
```

Once  everything  gets  compiled,   the  static  library `libns3ofswitch13.a` file  will  be  available  under  the `ofswitch13/lib/ofsoftswitch13/udatapath` directory.

## mmWave ns-3 Module v2.0

This is based on the mmWave [version 2.0](https://github.com/nyuwireless-unipd/ns3-mmwave/releases/tag/V2.0 "mmWave 2.0 github repository") module for the simulation of G mmWave cellular networks. A description of this module can be found on [IEEExplore (open access)](https://ieeexplore.ieee.org/document/8344116/ "mmwave paper").

The mmWave module for ns-3 can be used to simulate 5G cellular networks at mmWave frequencies. 
This module builds on top of the LTE one, and version 2.0 includes features such as:
- Support of a wide range of channel models, including the latest 3GPP model for frequency spectrum above 6 GHz. Ray tracing and measured traces can also be modeled.
- Custom PHY and MAC classes, inspired to the PHY and MAC of 3GPP NR. They support dynamic TDD, and are parameterized and highly customizable in order to be flexible enough for testing different designs.
- Custom schedulers for the dynamic TDD format
- Carrier Aggregation at the MAC layer
- Enhancements to the RLC layer with re-segmentation of packets for retransmissions
- Dual Connectivity with LTE base stations, with fast secondary cell handover and channel tracking
- Simulation of core network elements (with also the MME as a real node)

## OpenFlow ns-3 Module v1.3

This is the OFSwitch13 module, which enhances the [ns-3 Network Simulator][ns-3] with [OpenFlow 1.3][ofp13] capabilities, allowing ns-3 users to simulate Software-Defined Networks (SDN). 
This module implements the interface for interconnecting the ns-3 simulator to the [OpenFlow 1.3 Software Switch for ns-3][ofs13] (ofsoftswitch13) library. It is the library that, in fact, provides the switch implementation, the library for converting to/from OpenFlow 1.3 wire format, and the dpctl tool for configuring the switch from the console.

Please, visit the [OFSwitch13 project homepage][project] for detailed information on the module design, documentation, and *how to get started* tutorials. The code API documentation for the latest release of this project is available [here][apidoc].

## Ns-2 Mobility Helper with 3D Mobility

The  *Ns2MobilityHelper* was expanded to allow 3D mobility traces from [this][ns2mobility] repository.


## License ##

The cellular mmWave software is licensed under the terms of the [GNU GPLv2 license][gpl], as like as ns-3. See the LICENSE file for more details.
The OFSwitch13 module is free software, licensed under the [GNU GPLv2 license][gpl], and is publicly available for research, development, and use.


## The Network Simulator, Version 3 ##

Table of Contents:
------------------

1) An overview
2) Building ns-3
3) Running ns-3
4) Getting access to the ns-3 documentation
5) Working with the development version of ns-3

Note:  Much more substantial information about ns-3 can be found at
http://www.nsnam.org

#### 1) An Open Source project
------------------------------

ns-3 is a free open source project aiming to build a discrete-event
network simulator targeted for simulation research and education.
This is a collaborative project; we hope that
the missing pieces of the models we have not yet implemented
will be contributed by the community in an open collaboration
process.

The process of contributing to the ns-3 project varies with
the people involved, the amount of time they can invest
and the type of model they want to work on, but the current
process that the project tries to follow is described here:
http://www.nsnam.org/developers/contributing-code/

This README excerpts some details from a more extensive
tutorial that is maintained at:
http://www.nsnam.org/documentation/latest/

#### 2) Building ns-3
---------------------

The code for the framework and the default models provided
by ns-3 is built as a set of libraries. User simulations
are expected to be written as simple programs that make
use of these ns-3 libraries.

To build the set of default libraries and the example
programs included in this package, you need to use the
tool 'waf'. Detailed information on how use waf is 
included in the file doc/build.txt

However, the real quick and dirty way to get started is to
type the command
  ./waf configure --enable-examples
followed by
  ./waf 
(If errors occurred during the build process, type the following command
  CXXFLAGS="-Wall" ./waf  configure --enable-examples
followed by
  ./waf)
in the the directory which contains
this README file. The files built will be copied in the
build/ directory.

The current codebase is expected to build and run on the
set of platforms listed in the RELEASE_NOTES file.

Other platforms may or may not work: we welcome patches to 
improve the portability of the code to these other platforms. 

#### 3) Running ns-3
--------------------

On recent Linux systems, once you have built ns-3 (with examples
enabled), it should be easy to run the sample programs with the
following command, such as:

  ./waf --run simple-global-routing

That program should generate a simple-global-routing.tr text 
trace file and a set of simple-global-routing-xx-xx.pcap binary
pcap trace files, which can be read by tcpdump -tt -r filename.pcap
The program source can be found in the examples/routing directory.

#### 4) Getting access to the ns-3 documentation
------------------------------------------------

Once you have verified that your build of ns-3 works by running
the simple-point-to-point example as outlined in 4) above, it is
quite likely that you will want to get started on reading
some ns-3 documentation. 

All of that documentation should always be available from
the ns-3 website: http:://www.nsnam.org/documentation/.

This documentation includes:

  - a tutorial
 
  - a reference manual

  - models in the ns-3 model library

  - a wiki for user-contributed tips: http://www.nsnam.org/wiki/

  - API documentation generated using doxygen: this is
    a reference manual, most likely not very well suited 
    as introductory text:
    http://www.nsnam.org/doxygen/index.html

#### 5) Working with the development version of ns-3
----------------------------------------------------

If you want to download and use the development version 
of ns-3, you need to use the tool 'mercurial'. A quick and
dirty cheat sheet is included in doc/mercurial.txt but
reading through the mercurial tutorials included on the
mercurial website is usually a good idea if you are not
familiar with it.

If you have successfully installed mercurial, you can get
a copy of the development version with the following command:
"hg clone http://code.nsnam.org/ns-3-dev"

[ns-3]: https://www.nsnam.org
[ofp13]: https://www.opennetworking.org/sdn-resources/technical-library
[ofs13]: https://github.com/ljerezchaves/ofsoftswitch13
[project]: http://www.lrc.ic.unicamp.br/ofswitch13/
[apidoc]: http://www.lrc.ic.unicamp.br/ofswitch13/doc/html/index.html
[gpl]: http://www.gnu.org/copyleft/gpl.html
[ns2mobility]: https://github.com/ogbautista/Ns2MobilityHelper
