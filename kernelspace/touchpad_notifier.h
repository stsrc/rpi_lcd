#ifndef _TOUCHPAD_NOTIFIER_H_
#define _TOUCHPAD_NOTIFIER_H_
#include <linux/notifier.h>

extern int register_touchpad_notifier(struct notifier_block *nb);
extern int unregister_touchpad_notifier(struct notifier_block *nb);

#endif
