use std::{fs, time::Duration};

use std::time::{SystemTime, UNIX_EPOCH};

const ENABLE: &str = "/sys/bus/i2c/drivers/lcd-driver/enable";
const DISPLAY: &str = "/sys/bus/i2c/drivers/lcd-driver/display";

fn main() -> std::io::Result<()> {
    fs::write(ENABLE, b"1")?;

    let mut number = ((SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap()
        .as_nanos()
        % 13)
        + 1) as u8;

    loop {
        let result = match number {
            1 => "1".to_string(),
            2 => "2".to_string(),
            3 => "3".to_string(),
            4 => "4".to_string(),
            5 => "5".to_string(),
            6 => "6".to_string(),
            7 => "7".to_string(),
            8 => "8".to_string(),
            9 => "9".to_string(),
            10 => "10".to_string(),
            11 => "J".to_string(),
            12 => "Q".to_string(),
            _ => "K".to_string(),
        };

        let message = format!("Current card is {}.\nHigher or Lower?", result);
        fs::write(DISPLAY, message.as_bytes())?;

        let mut input: String = String::new();
        std::io::stdin().read_line(&mut input)?;

        if input == "Q\n" || input == "q\n" {
            fs::write(DISPLAY, b"Thanks for playing!")?;
            std::thread::sleep(Duration::from_secs(1));
            break;
        }

        let is_higher = match input.as_str() {
            "H\n" | "h\n" => Some(true),
            "L\n" | "l\n" => Some(false),
            _ => None,
        };

        if is_higher.is_none() {
            continue;
        }

        let old_number = number;
        number = ((SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos()
            % 13)
            + 1) as u8;

        if (old_number < number && is_higher.unwrap())
            || (old_number > number && !is_higher.unwrap())
            || (number == old_number)
        {
            fs::write(DISPLAY, b"Good Guess!")?;
            std::thread::sleep(Duration::from_secs(1));
        } else {
            fs::write(DISPLAY, b"Wrong Guess!")?;
            std::thread::sleep(Duration::from_secs(1));
        }
    }

    fs::write(ENABLE, b"0")?;

    Ok(())
}
