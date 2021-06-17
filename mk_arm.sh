	dos2unix $WORK_ROOT/src/3rd/ACE_wrappers/bin/add_rel_link.sh
	chmod +x $WORK_ROOT/src/3rd/ACE_wrappers/bin/add_rel_link.sh
	cd $WORK_ROOT/src/3rd
	make   sqlite_db=1 arm_plat=1 clean
	make all release=1 arm_plat=1  sqlite_db=1
	cd $WORK_ROOT/src
	make   sqlite_db=1 arm_plat=1 clean
	make all release=1 sqlite_db=1  arm_plat=1
	make pack
