#!/usr/bin/python
import _libRegex
reg = ["(aok.{4,5}r","(aok.{45sd}","(aok.{452937497237493}","","(aok.{6,5})","idy","..zef.",".","(a.{0,5})o."]
PERSO = "aok."
#print PERSO
print "--------------------"
print

def test(option):
    for r in reg:
        print "______________________"
        print "regex: "+r
        try:
            l = _libRegex.match(r, PERSO, option)
            if len(r) < 100:
                print r
            print "SUCCESS"
            print l
            print len(l)
        except Exception, e:
            print ": ".join(e)

test(0)
test(1)
