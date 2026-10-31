// SPDX-License-Identifier: BSD-2-Clause-Patent
#pragma once
// Experimental binary dispatch layout for the exact supplied iOS17 family.
// Slot probe variant: unclassified entries return kIOReturnUnsupported.
// Unclassified entries have no usable prototype or return contract. Derived
// implementations must fail without returning; never advertise them as supported.
#include <IOKit/IOService.h>
#include <IOKit/IOUserClient.h>
class IOMobileFramebuffer : public IOService {
    OSDeclareAbstractStructors(IOMobileFramebuffer)
    uint64_t familyPrivate;
public:
    virtual IOReturn unclassified168() = 0;
    virtual IOReturn unclassified169() = 0;
    virtual IOReturn unclassified170() = 0;
    virtual IOReturn unclassified171() = 0;
    virtual IOReturn unclassified172() = 0;
    virtual IOReturn unclassified173() = 0;
    virtual IOReturn unclassified174() = 0;
    virtual IOReturn unclassified175() = 0;
    virtual IOReturn unclassified176() = 0;
    virtual IOReturn unclassified177() = 0;
    virtual IOReturn unclassified178() = 0;
    virtual IOReturn unclassified179() = 0;
    virtual IOReturn unclassified180() = 0;
    virtual IOReturn unclassified181() = 0;
    virtual IOReturn beforeExternalMethod() = 0;
    virtual IOReturn unclassified183() = 0;
    virtual IOReturn unclassified184() = 0;
    virtual IOReturn unclassified185() = 0;
    virtual IOReturn unclassified186() = 0;
    virtual IOReturn unclassified187() = 0;
    virtual IOReturn unclassified188() = 0;
    virtual IOReturn unclassified189() = 0;
    virtual IOReturn unclassified190() = 0;
    virtual IOReturn unclassified191() = 0;
    virtual IOReturn unclassified192() = 0;
    virtual IOReturn releaseClientState(IOUserClient *) = 0;
    virtual IOReturn unclassified194() = 0;
    virtual IOReturn unclassified195() = 0;
    virtual IOReturn unclassified196() = 0;
    virtual IOReturn unclassified197() = 0;
    virtual IOReturn unclassified198() = 0;
    virtual IOReturn unclassified199() = 0;
    virtual IOReturn unclassified200() = 0;
    virtual IOReturn unclassified201() = 0;
    virtual IOReturn unclassified202() = 0;
    virtual IOReturn unclassified203() = 0;
    virtual IOReturn unclassified204() = 0;
    virtual IOReturn unclassified205() = 0;
    virtual IOReturn unclassified206() = 0;
    virtual IOReturn unclassified207() = 0;
    virtual IOReturn unclassified208() = 0;
    virtual IOReturn unclassified209() = 0;
    virtual IOReturn unclassified210() = 0;
    virtual IOReturn unclassified211() = 0;
    virtual IOReturn unclassified212() = 0;
    virtual IOReturn unclassified213() = 0;
    virtual IOReturn unclassified214() = 0;
    virtual IOReturn unclassified215() = 0;
    virtual IOReturn unclassified216() = 0;
    virtual IOReturn unclassified217() = 0;
    virtual IOReturn unclassified218() = 0;
    virtual IOReturn unclassified219() = 0;
    virtual IOReturn unclassified220() = 0;
    virtual IOReturn unclassified221() = 0;
    virtual IOReturn unclassified222() = 0;
    virtual IOReturn unclassified223() = 0;
    virtual IOReturn unclassified224() = 0;
    virtual IOReturn unclassified225() = 0;
    virtual IOReturn unclassified226() = 0;
    virtual IOReturn unclassified227() = 0;
    virtual IOReturn unclassified228() = 0;
    virtual IOReturn unclassified229() = 0;
    virtual IOReturn unclassified230() = 0;
    virtual IOReturn unclassified231() = 0;
    virtual IOReturn unclassified232() = 0;
    virtual IOReturn unclassified233() = 0;
    virtual IOReturn unclassified234() = 0;
    virtual IOReturn unclassified235() = 0;
    virtual IOReturn unclassified236() = 0;
    virtual IOReturn unclassified237() = 0;
    virtual IOReturn unclassified238() = 0;
    virtual IOReturn unclassified239() = 0;
    virtual IOReturn unclassified240() = 0;
    virtual IOReturn unclassified241() = 0;
    virtual IOReturn unclassified242() = 0;
    virtual IOReturn unclassified243() = 0;
    virtual IOReturn unclassified244() = 0;
    virtual IOReturn unclassified245() = 0;
    virtual IOReturn unclassified246() = 0;
    virtual IOReturn unclassified247() = 0;
    virtual IOReturn unclassified248() = 0;
    virtual void getDisplayDimensions(uint32_t *, uint32_t *) = 0;
    virtual IOReturn unclassified250() = 0;
    virtual IOReturn unclassified251() = 0;
    virtual IOReturn unclassified252() = 0;
    virtual IOReturn unclassified253() = 0;
    virtual IOReturn unclassified254() = 0;
    virtual IOReturn unclassified255() = 0;
    virtual IOReturn unclassified256() = 0;
    virtual IOReturn unclassified257() = 0;
    virtual IOReturn unclassified258() = 0;
    virtual IOReturn unclassified259() = 0;
    virtual IOReturn unclassified260() = 0;
    virtual IOReturn unclassified261() = 0;
};
static_assert(sizeof(IOService)==136 && sizeof(IOMobileFramebuffer)==144);
