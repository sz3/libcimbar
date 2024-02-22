import subprocess
from subprocess import PIPE
from os.path import join as path_join, getsize
from unittest import TestCase
from unittest.mock import patch

from helpers import TestDirMixin, CIMBAR_SRC, BIN_DIR


CIMBAR_EXE = path_join(BIN_DIR, 'cimbar')


def _get_command(*args):
    cmd = [CIMBAR_EXE]
    for arg in args:
        if isinstance(arg, str):
            cmd += arg.split(' ')
        else:
            cmd += arg
    return cmd

# look in dist/bin/ for executables
# run them, confirm a few boring command results against samples(?)
# at the very least, confirm a roundtrip and cli options....


class CimbarCliTest(TestDirMixin, TestCase):
    def test_roundtrip_oneshot(self):
        # encode
        infile = path_join(CIMBAR_SRC, 'LICENSE')
        outprefix = path_join(self.working_dir.name, 'img')
        cmd = _get_command('--encode -i', infile, '-o', outprefix)

        res = subprocess.run(cmd, stdout=PIPE)
        self.assertEqual(0, res.returncode)

        encoded_img = f'{outprefix}_0.png'
        self.assertTrue(getsize(encoded_img) > 30000)

        # decode
        cmd = _get_command('-i', encoded_img, '-o', self.working_dir.name, '--no-deskew')
        res = subprocess.run(cmd, stdout=PIPE)
        self.assertEqual(0, res.returncode)

        # filename in stdout
        decoded_file = res.stdout.strip().decode('utf-8')
        self.assertTrue(self.working_dir.name in decoded_file)

        with open(infile) as r:
            expected = r.read()
        with open(decoded_file) as r:
            actual = r.read()

        self.assertEqual(expected, actual)

    def test_roundtrip_stdin(self):
        # encode
        infile = path_join(CIMBAR_SRC, 'LICENSE')
        outprefix = path_join(self.working_dir.name, 'img')
        cmd = _get_command('--encode -o', outprefix)

        res = subprocess.run(
            cmd, input=infile.encode('utf-8'), stdout=PIPE
        )

        self.assertEqual(0, res.returncode)

        encoded_img = f'{outprefix}_0.png'
        self.assertTrue(getsize(encoded_img) > 30000)

        # decode defaults to cwd -- which should be self.working_dir
        cmd = _get_command('--no-deskew')
        res = subprocess.run(
            cmd, input=encoded_img.encode('utf-8'), stdout=PIPE,
            cwd=self.working_dir.name,
        )
        self.assertEqual(0, res.returncode)

        # filename in stdout
        decoded_file = res.stdout.strip().decode('utf-8')
        self.assertTrue(self.working_dir.name in decoded_file)

        with open(infile) as r:
            expected = r.read()
        with open(decoded_file) as r:
            actual = r.read()

        self.assertEqual(expected, actual)
