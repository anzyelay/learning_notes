static int ds_system_call(const char *command, char *result_buf, unsigned int result_len)
{
    int result = -1;
    FILE *stream = NULL;

    if (command == null) {
        printf("the command is null\n");
        return result;
    }

    /* Open the command for reading. */
    stream = popen(command, "r");
    if (stream == NULL) {
        printf("system command failed");
        return result;
    }

    /* Read the output a line at a time - output it. */
    if (fgets(result_buf, result_len, stream) != NULL) {
        if(strlen(result_buf)>1 && result_buf[strlen(result_buf)-1]=='\n')
            result_buf[strlen(result_buf)-1] = '\0';
    }

    /* close */
    result = pclose(stream);
    if (WIFEXITED(result)) {
        result = WEXITSTATUS(result);
    }

    if (result != 0)
        printf("system command failed, rc=%d errno: %d \n", result, errno);

    return result;
}

/*
 * Execute Command
 *
 * Executes the command pointed to by cmd.  Output from the
 * executed command is captured and sent to LogCat Info.  Once
 * the command has finished execution, it's exit status is captured
 * and checked for an exit status of zero.  Any other exit status
 * causes diagnostic information to be printed and an immediate
 * testcase failure.
 */
void testExecCmd(const char *cmd)
{
    FILE *fp;
    int rv;
    int status;
    char str[200];

    // Display command to be executed
    printf("cmd: %s", cmd);

    // Execute the command
    fflush(stdout);
    if ((fp = popen(cmd, "r")) == NULL) {
        printf("execCmd popen failed, errno: %i", errno);
        exit(100);
    }

    // Obtain and display each line of output from the executed command
    while (fgets(str, sizeof(str), fp) != NULL) {
        if ((strlen(str) > 1) && (str[strlen(str) - 1] == '\n')) {
            str[strlen(str) - 1] = '\0';
        }
        printf(" out: %s", str);
    }

    // Obtain and check return status of executed command.
    // Fail on non-zero exit status
    status = pclose(fp);
    if (!(WIFEXITED(status) && (WEXITSTATUS(status) == 0))) {
        printf("Unexpected command failure");
        printf("  status: %#x", status);
        if (WIFEXITED(status)) {
            printf("WEXITSTATUS: %i", WEXITSTATUS(status));
        }
        if (WIFSIGNALED(status)) {
            printf("WTERMSIG: %i", WTERMSIG(status));
        }
        exit(101);
    }
}
