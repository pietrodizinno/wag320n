#---------------
SUB =
SUB += source
#SUB += source_smbpasswd
.EXPORT_ALL_VARIABLES:
#---------------
all:	${SUB}
#---------------
# ${SUB}::
#	${MAKE} ${MFLAGS} -C $@ || exit 1
#---------------
all install:
	for i in $(SUB) ; do $(MAKE) -C $$i $@ || exit $? ; done
#---------------
clean:
	rm -f *.[oa] *~ core
	for i in $(SUB) ; do ${MAKE} -C $$i clean || exit 1; done

#---------------
# End of file Makefile
