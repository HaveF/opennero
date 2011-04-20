"""

  This is a sample template for the log config file. Copy this file to 'logConfig.py' to have
  the system recognize your default logging type specifications.

"""


# This variable specifies which typed log messages that the client should listen to and report
ignore_types = []

# append a type to listen to
def ignore(type):
    ignore_types.append(type)

# These variables below describe per developer which types of log messages they want to ignore.
# For example, if I developer wishes to not see anything related to audio and ai, he would add
# ignore('ai') and ignore('audio'). Doing this achieves filtered logs.

# component section filters
ignore('ai.tick')
ignore('ai')
ignore('audio')
ignore('game')
#ignore('python')