#ifndef MELODY_PM_H
#define MELODY_PM_H

/* melody init — create melody.json + melody_modules/ in cwd */
int pm_init(void);

/* melody install github:user/repo[@version] */
int pm_install(const char* source);

/* melody install (no args) — install all from melody.json */
int pm_install_all(void);

/* melody uninstall <package-name> */
int pm_uninstall(const char* name);

#endif
