#include "diablo.h"
#include "storm/storm.h"
#include "ui/common.h"

BYTE movie_playing;
BOOL loop_movie;

void play_movie(const char *pszMovie, BOOL user_can_close)
{
  queue_video_state(pszMovie, user_can_close, loop_movie);
}
