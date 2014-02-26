
#ifndef PSXPLUGIN_H
#define PSXPLUGIN_H

#include "../../ChipPlugin.h"

namespace chipmachine {

class SexyPSFPlugin : public ChipPlugin {
public:
	virtual bool canHandle(const std::string &name) override;
	virtual ChipPlayer *fromFile(const std::string &name) override;
};

}

#endif // PSXPLUGIN_H