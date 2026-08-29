#---------------------------------------------------------------------------------------------------------------------
# Thomas's Bridge — Butano / GBA Makefile
#---------------------------------------------------------------------------------------------------------------------

TARGET       :=  thomas_bridge
BUILD        :=  build
ifeq ($(wildcard vendor/butano/butano/butano.mak),)
LIBBUTANO    :=  ../butano
else
LIBBUTANO    :=  vendor/butano/butano
endif
PYTHON       :=  python
SOURCES      :=  src src/screens src/core src/session src/content
INCLUDES     :=  include
DATA         :=
GRAPHICS     :=  graphics
AUDIO        :=  audio
AUDIOBACKEND :=  maxmod
AUDIOTOOL    :=
DMGAUDIO     :=  dmg_audio
DMGAUDIOBACKEND := null
ROMTITLE     :=  THOMASBRIDGE
ROMCODE      :=  THOM
USERFLAGS    :=  -DBN_CFG_SPRITE_AFFINE_MATS_MAX_ITEMS=32 -DBN_CFG_SPRITES_MAX_SORT_LAYERS=32
USERCXXFLAGS :=  -std=c++20
USERASFLAGS  :=
USERLDFLAGS  :=
USERLIBDIRS  :=
USERLIBS     :=
DEFAULTLIBS  :=
STACKTRACE   :=
USERBUILD    :=
EXTTOOL      :=

ifndef LIBBUTANOABS
    export LIBBUTANOABS := $(realpath $(LIBBUTANO))
endif

include $(LIBBUTANOABS)/butano.mak
