#include <tinyalsa/asoundlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <string.h>
#include <errno.h>

/**
 * vimrc
 * set ts=4
 * set sw=4
 */

static int capturing = 1;
void sigint_handler(int sig __unused)
{
	capturing = 0;
}

static unsigned int read_write_sample(unsigned int in_card,
				      unsigned int in_device,
				      unsigned int out_card,
				      unsigned int out_device,
				      unsigned int channels,
				      unsigned int rate,
				      enum pcm_format format,
				      unsigned int period_size,
				      unsigned int period_count)
{
	struct pcm_config config;
	struct pcm *pcm_in, *pcm_out;
	char *buffer;
	unsigned int size;
	unsigned int bytes_read = 0;

	memset(&config, 0, sizeof(config));
	config.channels = channels;
	config.rate = rate;
	config.period_size = period_size;
	config.period_count = period_count;
	config.format = format;
	config.start_threshold = 0;
	config.stop_threshold = 0;
	config.silence_threshold = 0;

	/* in path */
	pcm_in = pcm_open(in_card, in_device, PCM_IN, &config);
	if (!pcm_in || !pcm_is_ready(pcm_in)) {
		fprintf(stderr,
				"Unable to open PCM IN device (%s)\n", pcm_get_error(pcm_in));
		return 0;
	}

	/* out path */
	pcm_out = pcm_open(out_card, out_device, PCM_OUT, &config);
	if (!pcm_out || !pcm_is_ready(pcm_out)) {
		fprintf(stderr,
				"Unable to open PCM OUT device (%s)\n", pcm_get_error(pcm_out));
		return 0;
	}

	size = pcm_frames_to_bytes(pcm_in, pcm_get_buffer_size(pcm_in));
	printf("buffer size %d\n", size);
	printf("pcm_get_buffer_size %d\n", pcm_get_buffer_size(pcm_in));
	buffer = malloc(size);
	if (!buffer) {
		fprintf(stderr, "Unable to allocate %d bytes\n", size);
		pcm_close(pcm_in);
		pcm_close(pcm_out);
		return 0;
	}

	printf("Read sample: %u ch, %u hz, %u bit from /dev/pcmC%dD%dc\n", channels,
		   rate, pcm_format_to_bits(format), in_card, in_device);
	printf("Write sample: %u ch, %u hz, %u bit to /dev/pcmC%dD%dp\n", channels,
		   rate, pcm_format_to_bits(format), out_card, out_device);

	while (capturing && !pcm_read(pcm_in, buffer, size)) {
		if (pcm_write(pcm_out, buffer, size)) {
			fprintf(stderr, "Unable to pcm_write (%s)\n", pcm_get_error(pcm_out));
			break;
		}
		bytes_read += size;
	}

	free(buffer);

	pcm_close(pcm_out);
	pcm_close(pcm_in);

	return pcm_bytes_to_frames(pcm_in, bytes_read);
}

/*
 * The purpose of this test code
 * read general input path and write to nx_feedback path
 * channel, samplerate, bits must be same
 * card, device identifier: /dev/snd/pcmC{CARD}D{DEVICE}{c|p}
 * must argument
 * -cin: input path card number
 * -din: input path device number
 * -cout: out path card number
 * -dout: out path device number
 */
int main(int argc, char **argv)
{
	unsigned int in_card = 0;
	unsigned int in_device = 0;
	unsigned int out_card = 1;
	unsigned int out_device = 1;
	unsigned int channels = 2;
	unsigned int rate = 48000;
	unsigned int bits = 16;
	unsigned int frames;
	unsigned int period_size = 1024;
	unsigned int period_count = 4;
	enum pcm_format format;

	if (argc < 5) {
		fprintf(stderr,
			"Usage: %s -cin inputcard -din inputdevice -cout outputcard -dout outputdevice "
			"[-r rate] [-b bits] [-p period_size] [-n n_periods]\n",
			argv[0]);
		return -EINVAL;
	}

	/* parse command line arguments */
	argv += 1;
	while (*argv) {
		if (strcmp(*argv, "-cin") == 0) {
			argv++;
			if (*argv)
				in_card = atoi(*argv);
		} else if (strcmp(*argv, "-din") == 0) {
			argv++;
			if (*argv)
				in_device = atoi(*argv);
		} else if (strcmp(*argv, "-cout") == 0) {
			argv++;
			if (*argv)
				out_card = atoi(*argv);
		} else if (strcmp(*argv, "-dout") == 0) {
			argv++;
			if (*argv)
				out_device = atoi(*argv);
		} else if (strcmp(*argv, "-c") == 0) {
			argv++;
			if (*argv)
				channels = atoi(*argv);
        } else if (strcmp(*argv, "-r") == 0) {
            argv++;
            if (*argv)
                rate = atoi(*argv);
        } else if (strcmp(*argv, "-b") == 0) {
            argv++;
            if (*argv)
                bits = atoi(*argv);
        } else if (strcmp(*argv, "-p") == 0) {
            argv++;
            if (*argv)
                period_size = atoi(*argv);
        } else if (strcmp(*argv, "-n") == 0) {
            argv++;
            if (*argv)
                period_count = atoi(*argv);
		}
		if(*argv)
			argv++;
	}

	switch (bits) {
	case 32:
		format = PCM_FORMAT_S32_LE;
		break;
	case 24:
		format = PCM_FORMAT_S24_LE;
		break;
	case 16:
		format = PCM_FORMAT_S16_LE;
		break;
	default:
		fprintf(stderr, "%d bits is not supported.\n", bits);
		return 1;
	}

	signal(SIGINT, sigint_handler);
	frames = read_write_sample(in_card, in_device, out_card, out_device,
							   channels, rate, format,
							   period_size, period_count);

	printf("R/W %d frames\n", frames);
	return 0;
}
