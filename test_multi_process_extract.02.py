import datetime
import os
import shutil
import sys
import tarfile
import zipfile

now = datetime.datetime.now

BOOST_TAR = r"C:\src\NVIDIA\usd-ci\_build\src\boost_1_70_0.tar.gz"


_archiveMembersCache = {}


def _ArchiveSubsetExtract(archivePath, extractPath, start, end):
    global _archiveMembersCache
    archiveAndMembers = _archiveMembersCache.get(archivePath)
    if archiveAndMembers is None:
        archiveAndMembers = _PathToArchiveAndMembers(archivePath)
        _archiveMembersCache[archivePath] = archiveAndMembers
    archive, members = archiveAndMembers
    archive.extractall(extractPath, members=members[start:end])
    return end - start


def _PathToArchiveAndMembers(archivePath):
    if tarfile.is_tarfile(archivePath):
        archive = tarfile.open(archivePath)
        members = archive.getmembers()
    elif zipfile.is_zipfile(archivePath):
        archive = zipfile.ZipFile(archivePath)
        members = archive.namelist()
    else:
        raise RuntimeError("unrecognized archive file type")
    return archive, members


def IterChunkIndices(numItems, numChunks):
    lastIndex = 0
    total = 0
    for i in range(1, numChunks + 1):
        index = round(numItems * i / numChunks)
        yield lastIndex, index
        total += index - lastIndex
        lastIndex = index
    if total != numItems:
        raise RuntimeError(f"Chunked total, {total}, did not equal number of items, {numItems}")


def MultiProcessExtract(archivePath, extractPath, members=None):
    """
    Like archive.extractall, but attempts to use multiple processes

    On python-2, falls back to single-threaded behavior.
    """
    if sys.version_info.major < 3:
        # Not bothering to implement a version that works without
        # concurrent.futures, so just do normal single-threaded extractall
        SingleProcessExtract(archivePath, extractPath, members=members)
        return

    import concurrent.futures

    numMembers = len(_PathToArchiveAndMembers(archivePath)[1])

    futures = []
    with concurrent.futures.ProcessPoolExecutor() as executor:
        # Put into roughly _max_workers * 3 chunks - x3 is just a rough
        # guess at a value that will avoid too much overhead due to repeated
        # calls through the process pool, but alleviate potential for one
        # process getting a MUCH lengthier set of files to extract than another
        # (ie, if all the files in the start of the members list are much
        # bigger, for instance...)
        numChunks = min(numMembers, executor._max_workers)
        # numChunks = min(numMembers, executor._max_workers * 3)
        # numChunks = numMembers
        for start, end in IterChunkIndices(numMembers, numChunks):
            futures.append(executor.submit(_ArchiveSubsetExtract, archivePath, extractPath, start, end))
        finished = 0
        print(f"\rExtracted {finished:,} / {numMembers:,}", end="", flush=True)
        for future in concurrent.futures.as_completed(futures):
            finished += future.result()
            print(f"\rExtracted {finished:,} / {numMembers:,}", end="", flush=True)
        print()


def SingleProcessExtract(archivePath, extractPath, members=None):
    archive = _PathToArchiveAndMembers(archivePath)[0]
    archive.extractall(extractPath, members=members)


def time_extract(method, archive_path):
    archive_parent, archive_filename = os.path.split(os.path.normpath(os.path.abspath(archive_path)))
    archive_base = archive_filename.split(".")[0]
    extract_dirname = archive_base + "." + now().strftime("%Y-%m-%d_%H.%M.%S")
    extract_dir = os.path.join(archive_parent, extract_dirname)
    if os.path.isdir(extract_dir):
        shutil.rmtree(extract_dir)
    os.makedirs(extract_dir)

    print(f"Extracting {archive_path} using {method.__name__} to {extract_dir}")
    start = now()
    method(archive_path, extract_dir)
    end = now()
    elapsed = end - start

    print(f"Elapsed: {elapsed} - Start: {start} - End: {end}")
    return elapsed


def main():
    # time_extract(SingleProcessExtract, BOOST_TAR)
    time_extract(MultiProcessExtract, BOOST_TAR)


if __name__ == "__main__":
    main()
