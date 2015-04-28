


// public function prototypes

int init_rtklib_client(void);
int dismiss_rtklib_client(void);
void * read_frm_RTKlib(void);
float get_latitude(void *llh);
float get_longitude(void *llh);
float get_height(void *llh);
