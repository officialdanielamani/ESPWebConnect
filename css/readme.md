# Styling
On the `style.css` file just change the global variable to change the colour of system. In later update we also will include option such as size, width and height.

```CSS
:root {
    /* Global color settings */
	--background-color: #2b1e44; /* Dark purple, reflecting Keqing's main outfit */
  --text-color: #e0e0e0; /* Light gray for general content */
	/*More Color options...*/
	}
```

## Dark
Default colour theme.

[![Default](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Dark.png?raw=true "Default")](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Dark.png?raw=true "Default")

```CSS
:root {
    /* Global color settings */
    --background-color: #1a1a2e; /* Background color */
    --text-color: #dcd6f7; /* Text color */
    --header-text-color: #dcd6f7; /* Header text color */
    --input-border-color: #4e4e8a; /* Input border color */
    --input-background-color: #1a1a2e; /* Input background color */
    --button-text-color: #ffffff; /* Button text color */
    --button-background-color: #5c439a; /* Button background color */
    --button-hover-color: #4e4e8a; /* Button hover color */
    --toggle-on-background-color: #f4c542; /* Toggle background color when checked */
    --toggle-on-color: #ffffff; /* Toggle knob color when on */
    --toggle-off-background-color: #ccc; /* Toggle background color when unchecked */
    --toggle-off-color: #ffffff; /* Toggle knob color when off */
    --table-border-color: #4e4e8a; /* Table border color */
    --table-header-color: #826cd7; /* Table header background color */
    --card-background-color: #25274d; /* Card background */
    --icon-color: #f4c542; /* Icon color */
    --notification-background-color: #25274d; /* Notification background color */
    --notification-icon-color: #f4c542; /* Notification icon color */
    --close-button-color: #ff0000; /* Close button color */
    --slider-rail-color:  #826cd7; /* Slider rail color */
    --slider-thumb-color:#f4c542; /* Slider thumb color */
    --slider-thumb-border-color: #1a1a2e; /* Slider thumb border color */

    /* Font Size Variables */
    --base-font-size: 16px; /* Base font size for body */
    --h1-font-size: 2em;
    --h2-font-size: 1.75em;
    --h3-font-size: 1.25em;
    --p-font-size: 1em;
    --form-font-size: 1em;
    --input-font-size: 1em;
    --button-font-size: 1.25em;
    --button-card-font-size: 1.2em;
    --card-font-size: 1.2em;
    --table-font-size: 1em;
    --notification-font-size: 1em;
    --notification-message-font-size: 1.375em;
    --close-button-font-size: 1.25em;
}
```

## Light
Light mode to burn your eyes.

[![Light Mode](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Light.png?raw=true "Light Mode")](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Light.png?raw=true "Light Mode")

```CSS
:root {
    /* Global color settings */
    --background-color: #f5f5f5; /* Light gray for body background */
    --text-color: #333333; /* Dark gray for main text */
    --header-text-color: #111111; /* Almost black for headers */
    --input-border-color: #cccccc; /* Light gray for input borders */
    --input-background-color: #ffffff; /* White for input backgrounds */
    --button-text-color: #ffffff; /* White for button text */
    --button-background-color: #007bff; /* Medium blue for button background */
    --button-hover-color: #0056b3; /* Darker blue for button hover */
    --toggle-on-background-color: #4caf50; /* Green for toggle background when checked */
    --toggle-on-color: #ffffff; /* White for toggle knob when on */
    --toggle-off-background-color: #e0e0e0; /* Light gray for toggle background when unchecked */
    --toggle-off-color: #333333; /* Dark gray for toggle knob when off */
    --table-border-color: #dddddd; /* Light gray for table borders */
    --table-header-color: #f0f0f0; /* Very light gray for table headers */
    --card-background-color: #ffffff; /* White for card background */
    --icon-color: #007bff; /* Medium blue for icons */
    --notification-background-color: #cccccc; /* Bit Gray for notifications */
    --notification-icon-color: #333333; /* Dark gray for notification icons */
    --close-button-color: #f44336; /* Red for close button */
}

```

### Keqing
Dark mode friendly, purple based on Keqing from Genshin Impact.

[![Keqing](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Keqing.png?raw=true "Keqing")](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Keqing.png?raw=true "Keqing")

```CSS
:root {
    /* Global color settings */
    --background-color: #1a1a2e; /* Background color */
    --text-color: #dcd6f7; /* Text color */
    --header-text-color: #dcd6f7; /* Header text color */
    --input-border-color: #4e4e8a; /* Input border color */
    --input-background-color: #1a1a2e; /* Input background color */
    --button-text-color: #ffffff; /* Button text color */
    --button-background-color: #5c439a; /* Button background color */
    --button-hover-color: #4e4e8a; /* Button hover color */
    --toggle-on-background-color: #f4c542; /* Toggle background color when checked */
    --toggle-on-color: #ffffff; /* Toggle knob color when on */
    --toggle-off-background-color: #ccc; /* Toggle background color when unchecked */
    --toggle-off-color: #ffffff; /* Toggle knob color when off */
    --table-border-color: #4e4e8a; /* Table border color */
    --table-header-color: #826cd7; /* Table header background color */
    --card-background-color: #25274d; /* Card background */
    --icon-color: #f4c542; /* Icon color */
    --notification-background-color: #25274d; /* Notification background color */
    --notification-icon-color: #f4c542; /* Notification icon color */
    --close-button-color: #ff0000; /* Close button color */
    --slider-rail-color:  #826cd7; /* Slider rail color */
    --slider-thumb-color:#f4c542; /* Slider thumb color */
    --slider-thumb-border-color: #1a1a2e; /* Slider thumb border color */

	/* Font setting follow example */
}
```

## Megumin
Dark mode friendly, Red-Yellow based on Megumin from Konosuba

[![Megumin](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Megumin.png?raw=true "Megumin")](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Megumin.png?raw=true "Megumin")

```CSS
:root {
    --background-color: #2e1b25; /* Dark red-black, matching Megumin's hat and cloak */
    --text-color: #e8d6c5; /* Light beige for general text */
    --header-text-color: #ffffff; /* White for headers */
    --input-border-color: #a43e47; /* Reddish for input borders */
    --input-background-color: #3b242e; /* Darker red for input backgrounds */
    --button-text-color: #ffffff; /* White for button text */
    --button-background-color: #d04e5b; /* Crimson red for button background */
    --button-hover-color: #b43a47; /* Darker red for button hover */
    --toggle-on-background-color: #f4c542; /* Bright yellow-gold for toggle when on */
    --toggle-on-color: #ffffff; /* White for toggle knob when on */
    --toggle-off-background-color: #55404c; /* Muted dark red for toggle when off */
    --toggle-off-color: #ffffff; /* White for toggle knob when off */
    --table-border-color: #a43e47; /* Reddish for table borders */
    --table-header-color: #4d2a34; /* Deep red for table headers */
    --card-background-color: #3b242e; /* Darker red for card background */
    --icon-color: #f4c542; /* Yellow-gold for icons */
    --notification-background-color: #4d2a34; /* Deep red for notification background */
    --notification-icon-color: #f4c542; /* Yellow-gold for notification icons */
    --close-button-color: #ff5c5c; /* Bright red for close button */
}
```

## Emilia

[![Emilia](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Emilia.png?raw=true "Emilia")](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/Emilia.png?raw=true "Emilia")

```CSS
:root {
    --background-color: #1e1e2f; /* Dark indigo, matching Emilia's darker palette elements */
    --text-color: #e3e7eb; /* Soft white-blue for general text */
    --header-text-color: #ffffff; /* Pure white for headers */
    --input-border-color: #6b7d9c; /* Muted blue-gray for input borders */
    --input-background-color: #2c2f48; /* Darker blue-gray for input backgrounds */
    --button-text-color: #ffffff; /* White for button text */
    --button-background-color: #a4b6e1; /* Light lavender-blue for button background */
    --button-hover-color: #8f9bc9; /* Slightly darker lavender-blue for button hover */
    --toggle-on-background-color: #c5c8f4; /* Soft lavender for toggle when on */
    --toggle-on-color: #ffffff; /* White for toggle knob when on */
    --toggle-off-background-color: #565b7b; /* Muted dark blue for toggle when off */
    --toggle-off-color: #ffffff; /* White for toggle knob when off */
    --table-border-color: #7984a8; /* Light blue-gray for table borders */
    --table-header-color: #3d4060; /* Darker blue-gray for table headers */
    --card-background-color: #2c2f48; /* Darker blue-gray for card background */
    --icon-color: #c5c8f4; /* Soft lavender for icons */
    --notification-background-color: #3d4060; /* Dark blue-gray for notification backgrounds */
    --notification-icon-color: #c5c8f4; /* Soft lavender for notification icons */
    --close-button-color: #f27979; /* Soft pinkish-red for close button */
}
```

## Roxy Migurdia

[![Roxy](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/RoxyMigurdia.png?raw=true "Roxy")](https://github.com/officialdanielamani/ESPWebConnect/blob/main/image/RoxyMigurdia.png?raw=true "Roxy")
```CSS

:root {
    --background-color: #1a2230; /* Deep navy blue, matching Roxy's darker clothing elements */
    --text-color: #dfe8f3; /* Light cyan for general text */
    --header-text-color: #ffffff; /* Pure white for headers */
    --input-border-color: #587aa4; /* Muted blue for input borders */
    --input-background-color: #24334c; /* Dark blue-gray for input backgrounds */
    --button-text-color: #ffffff; /* White for button text */
    --button-background-color: #6a98d0; /* Light sky blue for button background */
    --button-hover-color: #5884b9; /* Slightly darker sky blue for button hover */
    --toggle-on-background-color: #93c5fd; /* Soft azure blue for toggle when on */
    --toggle-on-color: #ffffff; /* White for toggle knob when on */
    --toggle-off-background-color: #3c506b; /* Muted dark blue for toggle when off */
    --toggle-off-color: #ffffff; /* White for toggle knob when off */
    --table-border-color: #5076a1; /* Blue-gray for table borders */
    --table-header-color: #2b3c58; /* Darker blue for table headers */
    --card-background-color: #24334c; /* Dark blue-gray for card background */
    --icon-color: #93c5fd; /* Azure blue for icons */
    --notification-background-color: #2b3c58; /* Dark blue for notification backgrounds */
    --notification-icon-color: #93c5fd; /* Azure blue for notification icons */
    --close-button-color: #ff7675; /* Soft coral red for close button */
}

```
