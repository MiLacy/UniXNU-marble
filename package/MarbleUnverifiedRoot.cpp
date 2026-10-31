// SPDX-License-Identifier: BSD-2-Clause-Patent
// Development-only volume lookup for the first unverified tail-root boot.
// The normal path authenticates every window through the owner policy.  This
// build deliberately has no policy, so it publishes the root as authorized and
// resolves the SystemOS APFS volume by its UUID without an authenticity check.
// Never ship this archive in a verified-root image.
#include <IOKit/IOBSD.h>
#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <libkern/c++/OSDictionary.h>
#include <libkern/c++/OSIterator.h>
#include <libkern/c++/OSNumber.h>
#include <libkern/c++/OSString.h>
#include <sys/conf.h>
#include <sys/systm.h>
#include <sys/types.h>

extern "C" int
native_verified_block_root_authorized(dev_t)
{
	return 1;
}

extern "C" int
native_verified_block_volume_device(const char *uuid, dev_t *device, char *path, size_t size)
{
	if (uuid == nullptr || device == nullptr || path == nullptr || size < 6 || strlen(uuid) != 36) {
		return 0;
	}
	*device = NODEV;
	path[0] = 0;
	OSDictionary *matching = IOService::serviceMatching("IOMedia");
	OSDictionary *properties = OSDictionary::withCapacity(1);
	OSString *wanted = OSString::withCString(uuid);
	if (matching == nullptr || properties == nullptr || wanted == nullptr) {
		OSSafeReleaseNULL(wanted);
		OSSafeReleaseNULL(properties);
		OSSafeReleaseNULL(matching);
		return 0;
	}
	bool ready = properties->setObject("UUID", wanted) &&
	    matching->setObject("IOPropertyMatch", properties);
	wanted->release();
	properties->release();
	OSIterator *iterator = ready ? IOService::getMatchingServices(matching) : nullptr;
	matching->release();
	if (iterator == nullptr) {
		return 0;
	}
	unsigned matches = 0;
	dev_t candidate = NODEV;
	char candidatePath[64] = {};
	while (OSObject *object = iterator->getNextObject()) {
		IOService *media = OSDynamicCast(IOService, object);
		if (media == nullptr) {
			continue;
		}
		if (++matches != 1) {
			break;
		}
		OSString *identity = OSDynamicCast(OSString, media->getProperty("UUID"));
		OSString *name = OSDynamicCast(OSString, media->getProperty(kIOBSDNameKey));
		OSNumber *major = OSDynamicCast(OSNumber, media->getProperty(kIOBSDMajorKey));
		OSNumber *minor = OSDynamicCast(OSNumber, media->getProperty(kIOBSDMinorKey));
		if (identity == nullptr || strcmp(identity->getCStringNoCopy(), uuid) != 0 ||
		    name == nullptr || major == nullptr || minor == nullptr || name->getLength() == 0 ||
		    name->getLength() >= sizeof(candidatePath) - 6 || name->getLength() >= size - 6 ||
		    major->unsigned64BitValue() > 255 || minor->unsigned64BitValue() > 0xffffff) {
			continue;
		}
		candidate = static_cast<dev_t>(makedev(major->unsigned32BitValue(), minor->unsigned32BitValue()));
		snprintf(candidatePath, sizeof(candidatePath), "/dev/%s", name->getCStringNoCopy());
	}
	iterator->release();
	if (matches != 1 || candidate == NODEV) {
		return 0;
	}
	*device = candidate;
	strlcpy(path, candidatePath, size);
	return 1;
}
