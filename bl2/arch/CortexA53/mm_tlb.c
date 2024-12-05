

int free_asp_pages(struct address_space *asp) {
	return 0;
}

int share_process_pages(struct task *to, struct task *from) {
	return 0;
}

int unmapmem(struct task *t, unsigned long int vaddr) {
	return 0;
}

int count_shared_pages() {
	return 0;
}

unsigned int get_pfn_sh(unsigned int ix) {
	return 0;
}

