use id3::{Tag, Version};



// const tag_version = Version::Id3v24;

fn main() {
    println!("Hello, world!");
}

fn get_tag(givenFile: String, tag: String)
{
    let tag = Tag::read_from_path(givenFile);

    // Get a bunch of frames...
    if let Some(artist) = tag.tag() {
        println!("artist: {}", artist);
    }
    if let Some(title) = tag.title() {
        println!("title: {}", title);
    }
    if let Some(album) = tag.album() {
        println!("album: {}", album);
    }
}

//Removes all tags from a provided file.
fn strip_tags(givenFile: String)
{
    let mut tag = Tag::new();
    tag.write_to_path(givenFile, Version::Id3v24);
}