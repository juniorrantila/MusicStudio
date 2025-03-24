#pragma once

typedef enum VSTExtensionVendor {
    VSTExtensionVendor_Prosonus = 'PreS'
} VSTExtensionVendor;

typedef enum VSTProsonusPluginEvent {
    VSTProsonusPluginEvent_EditSetRect = 'AeEm',
    VSTProsonusPluginEvent_EditSetContentScaleFactor = 'AeCs',
} VSTProsonusPluginEvent;
