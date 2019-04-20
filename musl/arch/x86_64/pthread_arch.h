extern struct pthread* libos_pthread_self(void);

static inline struct pthread *__pthread_self()
{
	struct pthread *self;
	//__asm__ ("mov %%fs:0,%0" : "=r" (self) );
	self = (struct pthread*)libos_pthread_self();
	return self;
}

#define TP_ADJ(p) (p)

#define MC_PC gregs[REG_RIP]
