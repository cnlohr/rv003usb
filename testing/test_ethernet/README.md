# Notes

This works with Windows, but not Linux.

Need to avoid anything that calls `usbnet_get_endpoints` because a rules-y person got their way.  Also can't use `cdc_ncm_find_endpoints` because it does the same.

Possible Linux interface candidates may include:
 * CDC-WDM (cdc-wdm.c)
 * cdc-phonet.c (Though it relies on alt settings)
 
