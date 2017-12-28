include ../makefile.inc

all: librbf.a rbftest1 rbftest2 rbftest3 rbftest4 rbftest5 rbftest6 rbftest7 rbftest8 rbftest8b rbftest9 rbftest10 rbftest11 rbftest12 rbftest_p0 rbftest_p1 rbftest_p1b rbftest_p1c rbftest_p2 rbftest_p2b rbftest_p3 rbftest_p4 rbftest_p5

# c file dependencies
pfm.o: pfm.h
rbfm.o: rbfm.h

# lib file dependencies
librbf.a: librbf.a(pfm.o)  # and possibly other .o files
librbf.a: librbf.a(rbfm.o)

rbftest1.o: pfm.h rbfm.h
rbftest2.o: pfm.h rbfm.h
rbftest3.o: pfm.h rbfm.h
rbftest4.o: pfm.h rbfm.h
rbftest5.o: pfm.h rbfm.h
rbftest6.o: pfm.h rbfm.h
rbftest7.o: pfm.h rbfm.h
rbftest8.o: pfm.h rbfm.h
rbftest8b.o: pfm.h rbfm.h
rbftest9.o: pfm.h rbfm.h
rbftest10.o: pfm.h rbfm.h
rbftest11.o: pfm.h rbfm.h
rbftest12.o: pfm.h rbfm.h
rbftest_p0.o: pfm.h rbfm.h
rbftest_p1.o: pfm.h rbfm.h
rbftest_p1b.o: pfm.h rbfm.h
rbftest_p1c.o: pfm.h rbfm.h
rbftest_p2.o: pfm.h rbfm.h
rbftest_p2b.o: pfm.h rbfm.h
rbftest_p3.o: pfm.h rbfm.h
rbftest_p4.o: pfm.h rbfm.h
rbftest_p5.o: pfm.h rbfm.h

# binary dependencies
rbftest1: rbftest1.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest2: rbftest2.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest3: rbftest3.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest4: rbftest4.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest5: rbftest5.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest6: rbftest6.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest7: rbftest7.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest8: rbftest8.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest8b: rbftest8b.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest9: rbftest9.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest10: rbftest10.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest11: rbftest11.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest12: rbftest12.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p0: rbftest_p0.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p1: rbftest_p1.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p1b: rbftest_p1b.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p1c: rbftest_p1c.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p2: rbftest_p2.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p2b: rbftest_p2b.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p3: rbftest_p3.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p4: rbftest_p4.o librbf.a $(CODEROOT)/rbf/librbf.a
rbftest_p5: rbftest_p5.o librbf.a $(CODEROOT)/rbf/librbf.a

# dependencies to compile used libraries
.PHONY: $(CODEROOT)/rbf/librbf.a
$(CODEROOT)/rbf/librbf.a:
	$(MAKE) -C $(CODEROOT)/rbf librbf.a

.PHONY: clean
clean:
	-rm rbftest1 rbftest2 rbftest3 rbftest4 rbftest5 rbftest6 rbftest7 rbftest8 rbftest8b rbftest9 rbftest10 rbftest11 rbftest12 rbftest_p0 rbftest_p1 rbftest_p1b rbftest_p1c rbftest_p2 rbftest_p2b rbftest_p3 rbftest_p4 rbftest_p5 *.a *.o *~
