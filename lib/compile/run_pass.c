/* @(#)run_pass.c 1.1 86/09/24 SMI */

#include "driver.h"
#include <sys/file.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>

#ifdef lint
	char		*sys_siglist[1];
	char		**environ;
#endif lint

extern	char	**environ;
extern	int	errno;

/**** expr ****/
static int expr(p) exprpt p;
{	int result[16], *sp= result, x, y;

	if (p == NULL) return(1);
	for (; 1; p++)
		switch (p->op) {
			case super_var_op:
				if (((super_intpt)p->value)->value == NULL) *sp++= 0;
				else *sp++= ((super_intpt)p->value)->value->value;
				break;
			case flag_var_op: *sp++= (is_on((int)(p->value)) ? 1:0); break;
			case var_op: *sp++= *p->value; break;
			case and_op: x= *--sp; y= *--sp; *sp++= x & y; break;
			case or_op: x= *--sp; y= *--sp; *sp++= x | y; break;
			case not_op: x= *--sp; *sp++= !x; break;
			case driver_op: *sp++= driver.value == (const_intpt)p->value ? 1:0; break;
			case target_op: *sp++= target.value == (const_intpt)p->value ? 1:0; break;
			case end_op: return(*--sp);};
#ifdef lint
	return(17);
#endif lint
}	/* expr */

/**** outfile_name ****/
char *outfile_name(source, out_suffix, programp) char *source; suffixpt out_suffix; programpt programp;
{	char *p, *result;

	if (build_lint_lib && (programp == &program.lint1))
		return("");

	if ((outfile != NULL) && (produce.value != &executable)) {
		if (is_on(used_outfile)) {
			set_flag(used_outfile);
			(void)fflush(stdout);
			(void)fprintf(stderr, "%s: Used -o name for first file only\n", program_name);
			(void)fflush(stderr);}
		else {
			if (strcmp(get_file_suffix(outfile), out_suffix->suffix))
				fatal1("Suffix mismatch between -o and produced file (should produce .%s)\n",
					out_suffix->suffix)
			else
				result= outfile;
				goto exit_fn;};};
	source= ((p= rindex(source, '/')) == NULL) ? source : p+1;
	if ((p= rindex(source, '.')) != NULL) *p= 0;
	result= get_memory(strlen(source)+1+strlen(out_suffix->suffix));
	(void)sprintf(result, "%s.%s", source, out_suffix->suffix);
	if (p != NULL) *p= '.';
    exit_fn:
	if (!strcmp(result, source))
		fatal1("Outfile would overwrite infile %s\n", result);
	return(result);
}	/* outfile_name */

/**** temp_file_name ****/
char *temp_file_name(program, suffix, mark) programpt program; suffixpt suffix; char mark;
{	char *string, buffer[1024];

	if (mark == 0)
		(void)sprintf(buffer, "%s/%s.%d.%d.%s",
			temp_dir, program->name, process_number, temp_file_number++, suffix->suffix);
	else
		(void)sprintf(buffer, "%s/%s.%d.%c.%d.%s",
			temp_dir, program->name, process_number, mark, temp_file_number++, suffix->suffix);
	string= get_memory(strlen(buffer)+1);
	(void)strcpy(string, buffer);
	return(add_to_list_tail(&files_to_unlink, ((char *)-((int)string))));
}	/* temp_file_name */

/**** wait_program ****/
static int wait_program(wait_step, step) steppt wait_step, step;
{	union wait wait_status;
	struct timeval stop;
	struct rusage usage; struct timeval cpu, elapsed;
	int process;
	steppt p, q;

	process= wait3(&wait_status, 0, &usage);
	for (p= wait_step; p <= step; p++)
		if (p->process == process) break;
	p->process= -1;
	processes_running--;
	if ((wait_status.w_T.w_Termsig != 0) || (wait_status.w_T.w_Retcode))
		for (q= wait_step; q <= step; q++)
			if (q->process > 0) {
				(void)kill(q->process, SIGKILL);
				q->killed= 1;};
	if (is_on(time_run)) {
		(void)gettimeofday(&stop, (struct timezone *)NULL);
		cpu.tv_sec= usage.ru_utime.tv_sec+usage.ru_stime.tv_sec;
		cpu.tv_usec= usage.ru_utime.tv_usec+usage.ru_stime.tv_usec;
		if (cpu.tv_usec > 1000000) {
			cpu.tv_usec-= 1000000; cpu.tv_sec++;};
		elapsed.tv_sec= stop.tv_sec-p->start.tv_sec;
		elapsed.tv_usec= stop.tv_usec-p->start.tv_usec;
		if (elapsed.tv_usec < 0) {
			elapsed.tv_usec+= 1000000; elapsed.tv_sec--;};
		(void)fflush(stdout);
		(void)fprintf(stderr, "%s: time U:%d.%01ds+S:%d.%01ds=%d.%01ds REAL:%d.%01ds %d%%. ",
			p->program->name,
			usage.ru_utime.tv_sec, usage.ru_utime.tv_usec/100000,
			usage.ru_stime.tv_sec, usage.ru_stime.tv_usec/100000,
			cpu.tv_sec, cpu.tv_usec/100000,
			elapsed.tv_sec, elapsed.tv_usec/100000,
			(cpu.tv_sec*1000+cpu.tv_usec/1000)*100/
				(elapsed.tv_sec*1000+elapsed.tv_usec/1000));
		(void)fprintf(stderr, "core T:%dk D:%dk. io IN:%db OUT:%db. pf IN:%dp OUT:%dp.\n",
			usage.ru_ixrss/1024, usage.ru_idrss/1024,
			usage.ru_inblock, usage.ru_oublock,
			usage.ru_majflt, usage.ru_minflt);};
	if (!p->killed && (wait_status.w_T.w_Termsig != 0)) {
		(void)fprintf(stderr, "%s: Fatal error in %s: %s%s\n",
			program_name,
			p->program->name,
			sys_siglist[wait_status.w_T.w_Termsig],
			wait_status.w_T.w_Coredump?" (core dumped)":"");
		(void)fflush(stderr);
		return(1);};
	return(wait_status.w_T.w_Retcode);
}	/* wait_program */

/**** use_temp_file ****/
static char *use_temp_file(file) char *file;
{	conspt tp;

	if (((int)file) < 0) {
		for (tp= files_to_unlink; tp != NULL; tp= tp->next)
			if (tp->value == file)
				return(tp->value= (char *)-((int)file));
		return((char *)-((int)file));};
	return(file);
}	/* use_temp_file */

/**** add_to_argv ****/
static void add_to_argv(p, consp) char ***p; conspt consp;
{
	for (;consp != NULL; consp= consp->next)
		*(*p)++= use_temp_file(consp->value);
}	/* add_to_argv */

/**** scan_path ****/
void scan_path(file, destination)
	char *file, *destination;
{	int result;

	if (program_path != NULL) { char *slash= rindex(file, '/');
		if (slash) {
			(void)strncpy(destination, file, slash-file);
			destination[slash-file]= 0;
			add_path_dir(destination, &program_path, last_program_path);
			result= access_pv(slash+1, 4, program_path, DEFAULT);}
		else
			result= access(file, 4);}
	else
		result= access(file, 4);
	if (result != 0) {
		if (is_on(dryrun))
			(void)strcpy(destination, file);
		else
			fatal1("Can not find %s\n", file);}
	else
		(void)strcpy(destination, vroot_data.full_path);
}	/* scan_path */

/**** run_steps ****/
void run_steps(source, step) char *source; steppt step;
{	steppt p, put= compile_stepsp, stop= NULL, wait_step= compile_steps;
	char *file= source, *setup_file= NULL;
	char **argv, **vp, *tp;
	int previous_pipe, this_pipe[2];
	char *std_out= NULL, *std_in= NULL;
	char filename[MAXPATHLEN];
	char *to_unlink[16];
	int to_unlink_cnt= 0;

	argv= (char **)alloca(arg_list_size*sizeof(char *));
/* First run thru the steps and find out where to stop */
	for (p= step; p->program != &program.sentinel_program_field; p++)
		if (expr(p->expr)) {
			if (p->suffix == requested_suffix) stop= put;
			*put++= *p;};
	if (stop == NULL)
		fatal2("Can not go from .%s to .%s\n", get_file_suffix(source), requested_suffix->suffix);
	(stop+1)->program= NULL;
/* Then run the steps */
	previous_pipe= -1;
	for (step= compile_steps; (step->program) && (status == 0); step++, setup_file= NULL) {
		step->program->outfile= NULL;
		if (step->setup != NULL)
			setup_file= (*step->setup)(source, file);
		if ((setup_file == NULL) && (step->program->setup != NULL))
			setup_file= (*step->program->setup)(source, file);
		if (setup_file != NULL)
			file= setup_file;
		else {
			(void)add_to_list_tail(&step->program->infile, file);
			if (((produce.value == &executable) && (step->suffix == &suffix.o)) ||
			    (((step+1)->program == NULL) &&
			     ((produce.value != &executable) || (driver.value == &m2c_driver)))) {
				file= outfile_name(source, requested_suffix, step->program);
				if ((source_infile_count == 1) && (produce.value == &executable) &&
				    (driver.value != &m2c_driver))
					(void)add_to_list_tail(&files_to_unlink, file);}
			else
				file= temp_file_name(step->program, step->suffix, 0);
			step->program->outfile= file;};
		vp= argv; *vp++= step->program->name;
		this_pipe[0]= this_pipe[1]= -1;
		std_in= std_out= NULL;
		for (tp= step->program->template; *tp != 0; tp++)
		    switch (*tp) {
			case IN_Z:
				add_to_argv(&vp, step->program->infile);
				if ((int)step->program->infile->value < 0)
					to_unlink[to_unlink_cnt++]= (char *)(-(int)step->program->infile->value);
				break;
			case STDIN_Z:
				if (previous_pipe == -1) {
					if ((int)step->program->infile->value < 0)
						to_unlink[to_unlink_cnt++]= (char *)-((int)step->program->infile->value);
					std_in= use_temp_file(step->program->infile->value);};
				break;
			case OUT_Z:
				*vp++= use_temp_file(step->program->outfile);
				break;
			case STDOUT_Z:
				if ((step+1)->program && ((step+1)->program->template[0] == STDIN_Z) && is_on(pipe_ok))
					(void)pipe(this_pipe);
				else
					if (step->program->outfile && !(build_lint_lib && (step->program == &program.lint1)))
						std_out= use_temp_file(step->program->outfile);
				break;
			case FLAG_Z:
				add_to_argv(&vp, step->program->flags);
				add_to_argv(&vp, step->program->permanent_flags); break;
			case MINUS_O_Z:
				*vp++= "-o";
				break;};
		*vp= NULL;
		scan_path(step->program->path, filename);
		(void)fflush(stdout);
		if (is_on(verbose) || is_on(dryrun) || is_on(time_run)) {
			(void)fprintf(stderr, "%s ", filename);
			for (vp= argv+1; *vp != NULL; vp++) { char *q; int space= 0;
				for (q= *vp; *q != 0; q++)
					if (isspace(*q)) { space= 1; break;};
				(void)fprintf(stderr, space ? "\"%s\" ":"%s ", *vp);};
			if (std_in != NULL)
				(void)fprintf(stderr, "< %s", std_in);
			if (build_lint_lib && (step->program == &program.lint1) && (this_pipe[0] == -1))
				(void)fprintf(stderr, ">> %s", outfile);
			if (std_out != NULL)
				(void)fprintf(stderr, "> %s", std_out);
			if (this_pipe[0] != -1)
				(void)fprintf(stderr, "|");
			else
				(void)fprintf(stderr, "\n");};
		(void)fflush(stderr);
		if (is_on(dryrun)) goto end_loop;
		processes_running++;
		if (is_on(time_run))
			(void)gettimeofday(&step->start, (struct timezone *)NULL);
		step->killed= 0;
		switch (step->process= fork()) {
		    case -1:
			fatal0("fork failed\n");
		    case 0:
			if (std_out != NULL) { int mode= 0666;
				if (!strncmp("/tmp", std_out, 4)) mode= 0600;
				if ((this_pipe[1]= open(std_out, O_WRONLY|O_CREAT|O_TRUNC, mode)) < 0) {
					(void)fprintf(stderr, "%s: Can not open %s for output\n", program_name, std_out);
					(void)fflush(stderr);
					_exit(100);};};
			if (this_pipe[1] > 0)
				(void)dup2(this_pipe[1], 1);
			if (build_lint_lib && (step->program == &program.lint1) && (std_out == NULL) && (this_pipe[1] <= 0))
				(void)dup2(build_lint_lib, 1);
			(void)close(this_pipe[0]);

			if (std_in != NULL)
				if ((previous_pipe= open(std_in, O_RDONLY, 0644)) < 0) {
					(void)fprintf(stderr, "%s: Can not open %s for input\n", program_name, std_in);
					_exit(100);};
			if (previous_pipe > 0)
				(void)dup2(previous_pipe, 0);
			execve_plain(filename, argv, environ);
			(void)fprintf(stderr, "%s: Can not exec %s\n", program_name, filename);
			(void)fflush(stderr);
			_exit(errno);
			break;
		    default:
			break;};
		(void)close(this_pipe[1]);
		if (this_pipe[0] == -1) {
			while (processes_running > 0) status|= wait_program(wait_step, step);
			wait_step= step;};
end_loop:
		if ((previous_pipe= this_pipe[0]) == -1) {
			for (to_unlink_cnt--; to_unlink_cnt>=0; to_unlink_cnt--) { conspt cp;
				if (is_on(verbose) || is_on(dryrun) || is_on(time_run)) {
					(void)fflush(stdout);
					(void)fprintf(stderr, "rm %s\n", to_unlink[to_unlink_cnt]);
					(void)fflush(stderr);};
				(void)unlink(to_unlink[to_unlink_cnt]);
				for (cp= files_to_unlink; cp != NULL; cp= cp->next)
					if (cp->value == to_unlink[to_unlink_cnt])
						cp->value= NULL;};
			to_unlink_cnt= 0;};};
	step--;
	if (step->suffix->collect)
		(*step->suffix->collect)(file, step->suffix);
}	/* run_steps */
