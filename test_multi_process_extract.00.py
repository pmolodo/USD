import datetime
import os
import shutil
import sys
import tarfile
import zipfile

now = datetime.datetime.now

BOOST_TAR = r"C:\src\NVIDIA\usd-ci\_build\src\boost_1_70_0.tar.gz"


_archiveCache = {}


def _archiveSingleMemberExtract(opener, archivePath, extractPath, member):
    global _archiveCache
    archive = _archiveCache.get(archivePath)
    if archive is None:
        archive = opener(archivePath)
        _archiveCache[archivePath] = archive
    archive.extract(member, extractPath)


def MultiProcessExtract(archive, archivePath, extractPath, members=None):
    """
    Like archive.extractall, but attempts to use multiple processes

    On python-2, falls back to single-threaded behavior.
    """
    if sys.version_info.major < 3:
        # Not bothering to implement a version that works without
        # concurrent.futures, so just do normal single-threaded extractall
        archive.extractall(extractPath, members=members)
        return

    import concurrent.futures

    if isinstance(archive, tarfile.TarFile):
        get_member_names = archive.getnames
        opener = tarfile.open
    elif isinstance(archive, zipfile.ZipFile):
        get_member_names = archive.namelist
        opener = zipfile.ZipFile
    else:
        raise RuntimeError("unrecognized archive file type")

    if members is None:
        members = get_member_names()

    numMembers = len(members)

    futures = []
    with concurrent.futures.ProcessPoolExecutor() as executor:
        for member in members:
            futures.append(executor.submit(_archiveSingleMemberExtract, opener, archivePath, extractPath, member))
        finished = 0
        print(f"\rExtracted {finished:,} / {numMembers:,}", end="", flush=True)
        for _ in concurrent.futures.as_completed(futures):
            finished += 1
            print(f"\rExtracted {finished:,} / {numMembers:,}", end="", flush=True)
        print()


def SingleProcessExtract(archive, archivePath, extractPath, members=None):
    archive.extractall(extractPath, members=members)


def time_extract(method, archive_path):
    archive_parent, archive_filename = os.path.split(os.path.normpath(os.path.abspath(archive_path)))
    archive_base = archive_filename.split(".")[0]
    extract_dirname = archive_base + "." + now().strftime("%Y-%m-%d_%H.%M.%S")
    extract_dir = os.path.join(archive_parent, extract_dirname)
    if os.path.isdir(extract_dir):
        shutil.rmtree(extract_dir)
    os.makedirs(extract_dir)

    if tarfile.is_tarfile(archive_path):
        archive = tarfile.open(archive_path)
        members = archive.getnames()
    elif zipfile.is_zipfile(archive_path):
        archive = zipfile.ZipFile(archive_path)
        members = archive.namelist()
    else:
        raise RuntimeError("unrecognized archive file type")

    print(f"Extracting {archive_path} using {method.__name__} to {extract_dir}")
    start = now()
    method(archive, archive_path, extract_dir)
    end = now()
    elapsed = end - start

    print(f"Elapsed: {elapsed} - Start: {start} - End: {end}")
    return elapsed


def main():
    # time_extract(SingleProcessExtract, BOOST_TAR)
    time_extract(MultiProcessExtract, BOOST_TAR)


if __name__ == "__main__":
    main()
