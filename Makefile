#---------------------------------------------------------------------------------------------------------------------
# Ryan's Bridge (leo_bridge) — Butano / GBA Makefile
#
# Expected layout (Butano template convention):
#   butano/
#     butano/          <- engine (LIBBUTANO)
#     leo_bridge/      <- this project
#
# Override the engine path if you keep the repo elsewhere:
#   make LIBBUTANO=/absolute/path/to/butano
#---------------------------------------------------------------------------------------------------------------------

TARGET       :=  leo_bridge
BUILD        :=  build
ifeq ($(wildcard vendor/butano/butano/butano.mak),)
LIBBUTANO    :=  ../butano
else
LIBBUTANO    :=  vendor/butano/butano
endif
PYTHON       :=  python
SOURCES      :=  src
INCLUDES     :=  include
DATA         :=
GRAPHICS     :=  graphics
AUDIO        :=  audio
AUDIOBACKEND :=  null
AUDIOTOOL    :=
DMGAUDIO     :=  dmg_audio
DMGAUDIOBACKEND := null
ROMTITLE     :=  RYANS BRIDGE
ROMCODE      :=  RYAN
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

#---------------------------------------------------------------------------------------------------------------------
# Export absolute butano path:
#---------------------------------------------------------------------------------------------------------------------
ifndef LIBBUTANOABS
    export LIBBUTANOABS := $(realpath $(LIBBUTANO))
endif

#---------------------------------------------------------------------------------------------------------------------
# Include main makefile:
#---------------------------------------------------------------------------------------------------------------------
include $(LIBBUTANOABS)/butano.mak
