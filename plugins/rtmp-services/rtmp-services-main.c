#include <util/text-lookup.h>
#include <util/threading.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <obs-module.h>
#include <file-updater/file-updater.h>

#include "rtmp-format-ver.h"
#include "lookup-config.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("rtmp-services", "en-US")

#define RTMP_SERVICES_LOG_STR "[rtmp-services plugin] "
#define RTMP_SERVICES_VER_STR "rtmp-services plugin (libobs " OBS_VERSION ")"

extern struct obs_service_info rtmp_custom_service;

static struct dstr module_name = {0};

const char *get_module_name(void)
{
	return module_name.array;
}

bool obs_module_load(void)
{
	dstr_copy(&module_name, "rtmp-services plugin (libobs ");
	dstr_cat(&module_name, obs_get_version_string());
	dstr_cat(&module_name, ")");

	obs_register_service(&rtmp_custom_service);
	return true;
}

void obs_module_unload(void)
{
	dstr_free(&module_name);
}
