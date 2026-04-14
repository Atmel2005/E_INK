#include "EPD_Base.h"
#include "EPD_SSD1681.h"
#include "EPD_SSD1608.h"
#include "EPD_UC8151.h"
#include "EPD_IL3829.h"
#include "EPD_SSD1680.h"

EPD_Base* createEPD(const EPDConfig& cfg) {
    switch (cfg.controller) {
        case EPDController::SSD1681: {
#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1681)
            return new EPD_SSD1681(cfg);
#else
            return nullptr;
#endif
        }
        case EPDController::SSD1680: {
#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1680)
            return new EPD_SSD1680(cfg);
#else
            return nullptr;
#endif
        }
            
        case EPDController::SSD1608: {
#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1608)
            return new EPD_SSD1608(cfg);
#else
            return nullptr;
#endif
        }
            
        case EPDController::UC8151: {
#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_UC8151)
            return new EPD_UC8151(cfg);
#elif !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1608)
            return new EPD_SSD1608(cfg);  // fallback
#else
            return nullptr;
#endif
        }
            
        case EPDController::IL3829: {
#if !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_IL3829)
            return new EPD_IL3829(cfg);
#elif !defined(EINK_SELECTIVE_BUILD) || defined(EINK_ENABLE_SSD1608)
            return new EPD_SSD1608(cfg);  // fallback
#else
            return nullptr;
#endif
        }
            
        default:
            return nullptr;  // Unsupported controller
    }
}
