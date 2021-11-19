# shellcheck shell=bash
# vim: filetype=sh

export HISTDB_ENABLED=1
export HISTDB_SESSION_ID=''
export HISTDB_LAST_COMMAND=''
# export HISTDB_COMMAND=~/bin/histdb

if ! hash histdb 2>/dev/null; then
    echo >&2 "error: histdb command not found"
fi

__histdb_is_int() { [[ -n $1 ]] && [[ $1 =~ ^[1-9][0-9]*$ ]]; }

__histdb_init_session_id() {
    local session_id
    session_id="$(HISTDB_PROD=1 histdb session)"
    if __histdb_is_int "${session_id}"; then
        export HISTDB_SESSION_ID=$session_id
    else
        echo >&2 "error: failed to generate a valid histdb session id: ${session_id}"
    fi
}

__histdb_check_session_id() {
    __histdb_is_int "${HISTDB_SESSION_ID}" || __histdb_init_session_id
}

__histdb_precmd() {
    if (( HISTDB_ENABLED == 1 )); then
        __histdb_check_session_id
        # This variable is set by the iTerm2 shell integration script, which
        # is based off of: https://github.com/rcaloras/bash-preexec
        #
        # shellcheck disable=SC2154
        local status_code=$__bp_last_ret_value
        local this_command
        this_command="$(HISTTIMEFORMAT='' builtin history 1)"
        if [[ $this_command != "${HISTDB_LAST_COMMAND}" ]]; then
            HISTDB_LAST_COMMAND=$this_command
            HISTDB_PROD=1 histdb insert \
                --session="${HISTDB_SESSION_ID}" \
                --status-code="${status_code}" \
                -- "${this_command}"
        fi
    fi
}

# WARN: using this for testing
histdb-enable() {
    __histdb_check_session_id
    export HISTDB_ENABLED=1
}

# WARN: using this for testing
histdb-disable() { export HISTDB_ENABLED=0; }

# TODO: check for preexec handling. This can be done checking if the function
# __bp_precmd_invoke_cmd is defined via `declare -f`, but requires that the
# iTerm2 shell integration is already loaded, which here it is not.

if [[ -v precmd_functions ]]; then
    precmd_functions+=(__histdb_precmd)
else
    precmd_functions=(__histdb_precmd)
fi

if (( HISTDB_ENABLED != 0 )); then
    # TODO: add "last RowID so that we can replicate bash's history"
    __histdb_check_session_id
fi
