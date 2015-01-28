#!/reg/g/pcds/package/python-2.5.2/bin/python
#
import pyami

class AmiScalar(pyami.Entry):
    def __init__(self,name):
        pyami.Entry.__init__(self,name)
    
class AmiAcqiris(pyami.Entry):
    def __init__(self,detid,channel):
        pyami.Entry.__init__(self,detid,channel)

eth_lo = 0x7f000001
eth_mc = 0xefff63e5
CxiAcq = 0x18000200
AmoETof = 0x03000200

pyami.connect(eth_mc,eth_lo,eth_lo)

x = pyami.Entry("ProcTime","Scan","EventId",20)
print x.get()

name_list = [("ProcLatency",),("ProcTime",),("ProcTimeAcc","Scan","EventId",20)]
x = pyami.EntryList(name_list)
print x.get()
print 'Again'
print x.get()

x = AmiAcqiris(AmoETof,1)
x.get()
