# loader-aslr-solver
This bruteforces the Switch RNG seed used by Loader for the ASLR codebin addrs, valid for all generated codebin addrs with official BootImagePackage until the next system (re)boot. At least 2 samples are needed (codebin addrs from different sysmodules), with only 1 sample many invalid seeds are found. Samples from an applet/application which was launched (multiple times) can also be used, if you add entries to the titles_loader_rng_maxval file for every process launched post-boot. If the same process was launched more than once, you need to use an unique id string in the titles_loader_rng_maxval and samples file, for each process.

These scripts assume AddressSpace64Bit, adjust as needed if that's not the case.

* Create file `launched_processes_list_{sysver}` where each line is a programid, beginning with the ProgramId for Boot2, then add the ProgramIds for your sysver from here: https://switchbrew.org/wiki/Boot2 Then add ProgramIds 0100000000001000 and 010000000000100C to the file.
* Then run `./sysupdate_get_nso_sizes.sh {sysupdatedir} > {sysupdatedir}/titles_hactool_bssend`. Then run `./get_loader_rng_max_from_titlelist.py launched_processes_list_{sysver} {path(s) to the previously generated titles_hactool_bssend files, starting with the one for the target sysver then older ones afterwards} > {sysupdatedir}/titles_loader_rng_maxval`
* Then create a samples file, where each line is: `{id matching an id from titles_loader_rng_maxval} {(codebin_baseaddr-0x8000000)>>21}` (adjust the latter if not AddressSpace64Bit)
* Then bruteforce (run without params for usage-listing): `./loader_aslr_solver --proclistrngmax {sysupdatedir}/titles_loader_rng_maxval --proclistsamples {above samples file}`
* If you want to generate RNG values with a specific seed, such as previously bruteforced seeds, run: `./loader_aslr_solver --proclistrngmax {sysupdatedir}/titles_loader_rng_maxval -S {seed}`
* Copy the output following the `RNG output...` line from the above into a file, then generate the ASLR codebin addr for each process by running: `./get_aslr_baseaddrs_from_samples.py {rngout_file}`

# Building
Building with libc++ is required for using the libc++ impl of `std::uniform_int_distribution`.

`clang++ -stdlib=libc++ -std=gnu++17 -Ofast -o loader_aslr_solver Mt19937.cpp loader_aslr_solver.cpp`

# Credits
* Includes code which is based on code by SciresM from elsewhere.

