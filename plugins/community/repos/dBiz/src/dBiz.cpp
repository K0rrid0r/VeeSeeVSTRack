#include "dBiz.hpp"
#include <math.h>

RACK_PLUGIN_MODEL_DECLARE(dBiz, dBizBlank);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Multiple);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Contorno);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Chord);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Utility);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Transpose);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Bene);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Bene2);
RACK_PLUGIN_MODEL_DECLARE(dBiz, BenePads);
RACK_PLUGIN_MODEL_DECLARE(dBiz, SubMix);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Remix);
RACK_PLUGIN_MODEL_DECLARE(dBiz, PerfMixer);
RACK_PLUGIN_MODEL_DECLARE(dBiz, VCA530);
RACK_PLUGIN_MODEL_DECLARE(dBiz, Verbo);
RACK_PLUGIN_MODEL_DECLARE(dBiz, DVCO);
RACK_PLUGIN_MODEL_DECLARE(dBiz, DAOSC);

RACK_PLUGIN_INIT(dBiz) {
   RACK_PLUGIN_INIT_ID();

   RACK_PLUGIN_MODEL_ADD(dBiz, dBizBlank);
   RACK_PLUGIN_MODEL_ADD(dBiz, Multiple);
   RACK_PLUGIN_MODEL_ADD(dBiz, Contorno);
   RACK_PLUGIN_MODEL_ADD(dBiz, Chord);
   RACK_PLUGIN_MODEL_ADD(dBiz, Utility);
   RACK_PLUGIN_MODEL_ADD(dBiz, Transpose);
   RACK_PLUGIN_MODEL_ADD(dBiz, Bene);
   RACK_PLUGIN_MODEL_ADD(dBiz, Bene2);
   RACK_PLUGIN_MODEL_ADD(dBiz, BenePads);
   RACK_PLUGIN_MODEL_ADD(dBiz, SubMix);
   RACK_PLUGIN_MODEL_ADD(dBiz, Remix);
   RACK_PLUGIN_MODEL_ADD(dBiz, PerfMixer);
   RACK_PLUGIN_MODEL_ADD(dBiz, VCA530);
   RACK_PLUGIN_MODEL_ADD(dBiz, Verbo);
   RACK_PLUGIN_MODEL_ADD(dBiz, DVCO);
   RACK_PLUGIN_MODEL_ADD(dBiz, DAOSC);
}
