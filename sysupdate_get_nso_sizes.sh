function process_titles
{
	for curdir in $1/*
	do
		if [[ -d $curdir ]]; then
			for regiondir in $curdir/*
			do
				if [[ -d $regiondir ]]; then
					region=$(basename "$regiondir")
					titlever=$(ls $regiondir/ | head -n 1)
					titledir="$regiondir/$titlever"
					titleid=$(basename $curdir)

					if [[ -d $titledir ]]; then
						filepath=$(ls -d $titledir/ncatype1_*.plain_section0_pfs0/main 2> /dev/null)

						if [[ -f $filepath ]]; then
							tmp=`hactool -t nso0 $filepath | grep .bss | cut -d- -f2`
							if [ $? -eq 0 ]; then
								echo "$titleid/$region/$titlever 0x$tmp"
							fi
						fi
					fi
				fi
			done
		fi
	done
}
						
process_titles $1
