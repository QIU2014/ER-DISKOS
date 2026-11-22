package com.eric;

import java.util.Calendar;
import java.util.Scanner;

public final class Main {
    private static String twoDigits(int value) {
        return value < 10 ? "0" + value : Integer.toString(value);
    }

    public static void main(String[] args) {
        System.out.println("Welcome to ER-DISKOS");

        Scanner scanner = new Scanner(System.in);
        while (true) {
            System.out.print("ER-DISKOS> ");
            String input = scanner.nextLine();
            if (input.equalsIgnoreCase("shutdown")) {
                System.out.println("Bye!");
                break;
            } else if (input.equalsIgnoreCase("help")) {
                System.out.println("Available commands:");
                System.out.println("  help - Show this help message");
                System.out.println("  shutdown - Shutdown the system");
                System.out.println("  echo <message> - Print the message to the console");
                System.out.println("  time - Show the current system time");
                System.out.println("  date - Show the current system date");
            } else if (input.startsWith("echo ")) {
                String message = input.substring(5);
                System.out.println(message);
            } else if (input.equalsIgnoreCase("time")) {
                Calendar now = Calendar.getInstance();
                if (now == null) {
                    System.out.println("Unable to read the system clock.");
                } else {
                    System.out.println("Current system time: "
                            + twoDigits(now.get(Calendar.HOUR_OF_DAY)) + ":"
                            + twoDigits(now.get(Calendar.MINUTE)) + ":"
                            + twoDigits(now.get(Calendar.SECOND)));
                }
            } else if (input.equalsIgnoreCase("date")) {
                Calendar now = Calendar.getInstance();
                if (now == null) {
                    System.out.println("Unable to read the system clock.");
                } else {
                    System.out.println("Current system date: "
                            + now.get(Calendar.YEAR) + "-"
                            + twoDigits(now.get(Calendar.MONTH) + 1) + "-"
                            + twoDigits(now.get(Calendar.DAY_OF_MONTH)));
                }
            } else {
                System.out.println("Unknown command: " + input);
            }
        }
        scanner.close();
    }
}
