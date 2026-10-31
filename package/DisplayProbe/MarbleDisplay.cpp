// SPDX-License-Identifier: BSD-2-Clause-Patent
// Slot probe for the original IOMobileFramebuffer family: every unclassified
// virtual records its ABI slot and reports unsupported, so one hardware boot
// names the driver entries the original client actually calls.
#include "MarbleDisplayABI.h"
#include <IOKit/IOLib.h>
#include <pexpert/pexpert.h>
extern "C" void boot_progress_stage(const char *message);

extern "C" uint64_t marble_display_calls;
extern "C" uint64_t marble_display_started;
extern "C" uint32_t marble_display_last;
extern "C" uint32_t marble_display_counts[262];

static void marble_slot(uint32_t slot)
{
	__atomic_fetch_add(&marble_display_calls, 1, __ATOMIC_RELAXED);
	__atomic_store_n(&marble_display_last, slot, __ATOMIC_RELAXED);
	if (slot < 262) {
		__atomic_fetch_add(&marble_display_counts[slot], 1, __ATOMIC_RELAXED);
	}
}

class MarbleDisplay : public IOMobileFramebuffer {
	OSDeclareDefaultStructors(MarbleDisplay)
public:
	bool start(IOService *provider) APPLE_KEXT_OVERRIDE;
	IOReturn beforeExternalMethod() APPLE_KEXT_OVERRIDE;
	IOReturn releaseClientState(IOUserClient *client) APPLE_KEXT_OVERRIDE;
	void getDisplayDimensions(uint32_t *width, uint32_t *height) APPLE_KEXT_OVERRIDE;
	IOReturn unclassified168() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified169() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified170() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified171() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified172() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified173() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified174() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified175() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified176() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified177() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified178() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified179() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified180() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified181() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified183() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified184() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified185() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified186() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified187() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified188() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified189() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified190() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified191() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified192() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified194() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified195() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified196() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified197() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified198() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified199() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified200() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified201() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified202() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified203() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified204() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified205() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified206() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified207() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified208() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified209() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified210() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified211() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified212() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified213() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified214() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified215() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified216() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified217() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified218() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified219() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified220() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified221() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified222() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified223() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified224() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified225() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified226() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified227() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified228() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified229() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified230() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified231() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified232() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified233() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified234() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified235() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified236() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified237() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified238() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified239() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified240() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified241() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified242() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified243() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified244() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified245() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified246() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified247() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified248() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified250() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified251() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified252() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified253() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified254() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified255() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified256() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified257() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified258() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified259() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified260() APPLE_KEXT_OVERRIDE;
	IOReturn unclassified261() APPLE_KEXT_OVERRIDE;
};

OSDefineMetaClassAndStructors(MarbleDisplay, IOMobileFramebuffer)

bool
MarbleDisplay::start(IOService *provider)
{
	PE_Video video;
	if (!IOService::start(provider)) {
		return false;
	}
	if (PE_current_console(&video) || !video.v_width || !video.v_height) {
		boot_progress_stage("marble display: no framebuffer");
		return false;
	}
	if (provider != nullptr) {
		provider->setProperty("canvas-width", video.v_width, 32);
		provider->setProperty("canvas-height", video.v_height, 32);
	}
	registerService();
	marble_display_started = 1;
	boot_progress_stage("marble display: service started");
	return true;
}

IOReturn
MarbleDisplay::beforeExternalMethod()
{
	return kIOReturnSuccess;
}

IOReturn
MarbleDisplay::releaseClientState(IOUserClient *client)
{
	if (client == nullptr || client->getProvider() != this) {
		return kIOReturnBadArgument;
	}
	return kIOReturnSuccess;
}

void
MarbleDisplay::getDisplayDimensions(uint32_t *width, uint32_t *height)
{
	PE_Video video;
	if (width && height && PE_current_console(&video) == false) {
		*width = (uint32_t)video.v_width;
		*height = (uint32_t)video.v_height;
		return;
	}
	if (width) {
		*width = 0;
	}
	if (height) {
		*height = 0;
	}
}

IOReturn
MarbleDisplay::unclassified168()
{
	marble_slot(168);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified169()
{
	marble_slot(169);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified170()
{
	marble_slot(170);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified171()
{
	marble_slot(171);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified172()
{
	marble_slot(172);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified173()
{
	marble_slot(173);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified174()
{
	marble_slot(174);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified175()
{
	marble_slot(175);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified176()
{
	marble_slot(176);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified177()
{
	marble_slot(177);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified178()
{
	marble_slot(178);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified179()
{
	marble_slot(179);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified180()
{
	marble_slot(180);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified181()
{
	marble_slot(181);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified183()
{
	marble_slot(183);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified184()
{
	marble_slot(184);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified185()
{
	marble_slot(185);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified186()
{
	marble_slot(186);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified187()
{
	marble_slot(187);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified188()
{
	marble_slot(188);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified189()
{
	marble_slot(189);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified190()
{
	marble_slot(190);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified191()
{
	marble_slot(191);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified192()
{
	marble_slot(192);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified194()
{
	marble_slot(194);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified195()
{
	marble_slot(195);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified196()
{
	marble_slot(196);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified197()
{
	marble_slot(197);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified198()
{
	marble_slot(198);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified199()
{
	marble_slot(199);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified200()
{
	marble_slot(200);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified201()
{
	marble_slot(201);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified202()
{
	marble_slot(202);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified203()
{
	marble_slot(203);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified204()
{
	marble_slot(204);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified205()
{
	marble_slot(205);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified206()
{
	marble_slot(206);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified207()
{
	marble_slot(207);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified208()
{
	marble_slot(208);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified209()
{
	marble_slot(209);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified210()
{
	marble_slot(210);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified211()
{
	marble_slot(211);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified212()
{
	marble_slot(212);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified213()
{
	marble_slot(213);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified214()
{
	marble_slot(214);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified215()
{
	marble_slot(215);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified216()
{
	marble_slot(216);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified217()
{
	marble_slot(217);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified218()
{
	marble_slot(218);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified219()
{
	marble_slot(219);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified220()
{
	marble_slot(220);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified221()
{
	marble_slot(221);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified222()
{
	marble_slot(222);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified223()
{
	marble_slot(223);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified224()
{
	marble_slot(224);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified225()
{
	marble_slot(225);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified226()
{
	marble_slot(226);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified227()
{
	marble_slot(227);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified228()
{
	marble_slot(228);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified229()
{
	marble_slot(229);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified230()
{
	marble_slot(230);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified231()
{
	marble_slot(231);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified232()
{
	marble_slot(232);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified233()
{
	marble_slot(233);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified234()
{
	marble_slot(234);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified235()
{
	marble_slot(235);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified236()
{
	marble_slot(236);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified237()
{
	marble_slot(237);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified238()
{
	marble_slot(238);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified239()
{
	marble_slot(239);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified240()
{
	marble_slot(240);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified241()
{
	marble_slot(241);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified242()
{
	marble_slot(242);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified243()
{
	marble_slot(243);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified244()
{
	marble_slot(244);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified245()
{
	marble_slot(245);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified246()
{
	marble_slot(246);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified247()
{
	marble_slot(247);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified248()
{
	marble_slot(248);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified250()
{
	marble_slot(250);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified251()
{
	marble_slot(251);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified252()
{
	marble_slot(252);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified253()
{
	marble_slot(253);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified254()
{
	marble_slot(254);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified255()
{
	marble_slot(255);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified256()
{
	marble_slot(256);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified257()
{
	marble_slot(257);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified258()
{
	marble_slot(258);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified259()
{
	marble_slot(259);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified260()
{
	marble_slot(260);
	return kIOReturnUnsupported;
}

IOReturn
MarbleDisplay::unclassified261()
{
	marble_slot(261);
	return kIOReturnUnsupported;
}

extern "C" uint64_t marble_display_calls;
extern "C" uint64_t marble_display_started;
extern "C" uint32_t marble_display_last;
extern "C" uint32_t marble_display_counts[262];
uint64_t marble_display_calls;
uint64_t marble_display_started;
uint32_t marble_display_last;
uint32_t marble_display_counts[262];

extern "C" void
native_mobile_display_provider_probe(void)
{
}
